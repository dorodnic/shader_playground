#include <iostream>
#include <chrono>
#include <iomanip>

#include "shader.h"
#include "window.h"
#include "texture.h"
#include "vao.h"
#include "camera.h"
#include "model.h"
#include "loader.h"
#include "advanced-shader.h"
#include "simple-shader.h"
#include "tube-shader.h"
#include "fbo.h"
#include "procedural.h"
#include "texture-2d-shader.h"
#include "glass-decals.h"
#include "textures.h"

#include <serializer.h>
#include <easylogging++.h>
INITIALIZE_EASYLOGGINGPP

#include <imgui.h>

#include <math.h>
#include <map>

#define LOG_GL_INT(x) { int max_textures = 0;\
glGetIntegerv(x, &max_textures);\
LOG(INFO) << #x << " = " << max_textures;\
}

struct graphic_objects
{
    std::shared_ptr<vao> earth, cat, grid;

    visualizer_2d viz;

    simple_shader shader;
    tube_shader tb_shader;
    texture_2d_shader tex_2d_shader;

    std::shared_ptr<fbo> background_pass, tubes_interior_pass;
};

struct tube_peice
{
    int type;
    int2 pos;
    bool damaged = false;
    int damaged_idx = 0;
    int glass_decal = 0;

    friend zpp::serializer::access;
    template <typename Archive, typename Self>
    static void serialize(Archive & archive, Self & self)
    {
        archive(self.type, self.pos.x, self.pos.y,
                self.damaged, self.damaged_idx, self.glass_decal);
    }
};

struct save_header
{
    int version = 1;

    friend zpp::serializer::access;
    template <typename Archive, typename Self>
    static void serialize(Archive & archive, Self & self)
    {
        archive(self.version);
    }
};

float3 mouse_pick(const int2& mouse,
    const int2& wh,
    const float4x4& proj,
    const float4x4& view)
{
    float2 normalized{
        2.f * mouse.x / wh.x - 1.f,
        1.0f - (2.0f * mouse.y) / wh.y,
    };
    float4 clipSpace{
        normalized.x,
        normalized.y,
        -1.f,
        1.f
    };
    auto invP = inverse(proj);
    auto eyeSpace = mul(invP, clipSpace);
    eyeSpace.z = -1.f;
    eyeSpace.w = 0.f;
    auto invV = inverse(view);
    return normalize(mul(invV, eyeSpace).xyz());
}

bool ImCheckButton(bool* state, const char* toggled_text, const char* untoggled_text)
{
    bool res = false;
    if (*state)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.8f, 0.f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.9f, 0.f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.4f, 0.f, 1.f));
        if (ImGui::Button(toggled_text, { 235, 0 }))
        {
            *state = false;
            res = true;
        }
        ImGui::PopStyleColor(3);
    }
    else
    {
        if (ImGui::Button(untoggled_text, { 235, 0 }))
            *state = true;
    }
    return res;
}

void load_tubes(std::vector<tube_peice>& tubes)
{
    std::vector<unsigned char> data;
    std::ifstream file("resources/save.dat", std::ios::in | std::ifstream::binary);
    assert(file.is_open());
    std::istreambuf_iterator<char> iter(file);
    std::copy(iter, std::istreambuf_iterator<char>{}, std::back_inserter(data));

    zpp::serializer::memory_input_archive in(data);

    save_header header{};
    save_header expected_header{};
    in(header);
    if (header.version == expected_header.version)
    {
        in(tubes);
    }
}

void save_tubes(const std::vector<tube_peice>& tubes)
{
    save_header header{};
    std::vector<unsigned char> data;
    zpp::serializer::memory_output_archive out(data);

    out(header);
    out(tubes);

    std::ofstream fout("resources/save.dat", std::ios::out | std::ios::binary);
    fout.write((char*)data.data(), data.size());
    fout.close();
}

int main(int argc, char* argv[])
{
    srand(time(NULL));

    START_EASYLOGGINGPP(argc, argv);

    auto app = std::make_unique<window>(1280, 720, "Shader Playground");

    float progress = 0.f;

    auto go = std::make_shared<graphic_objects>();

    bool mipmap = true;
    bool linear = true;

    textures textures;
    auto mish = textures.add_texture("diffuse", "resources/texture.png");
    auto normals = textures.add_texture("normal_map", "resources/normal_map.png");
    auto world = textures.add_texture("world diffuse", "resources/Diffuse_2K.png");
    auto cat_tex = textures.add_texture("cat diffuse", "resources/cat_diff.tga");
    auto white = textures.add_texture("white", "resources/white.png");

    auto first_pass_color = textures.add_texture("first_pass_color");
    auto second_pass_color = textures.add_texture("second_pass_color");

    auto empty_texture = textures.add_texture("empty");

    glass_atlas glass(textures);

    std::vector<tube_peice> tubes;

    auto reload_textures = [&]() {
        textures.reload_all();
    };

    loader ld("resources/earth.obj");
    while (!ld.ready());

    loader cat_ld("resources/cat.obj");
    while (!cat_ld.ready());

    light l;
    l.position = { 100.f, 0.f, -20.f };
    l.colour = { 1.f, 0.f, 0.f };

    auto shineDamper = 60.f;
    auto reflectivity = 0.5f;
    auto diffuse_level = 0.6f;
    auto light_angle = 0.f;
    bool rotate_light = true;
    float fov = 80.f;

    int a = 27;
    int b = 27;
    float radius = 1.f;
    float length = 3.f;

    auto cam = std::make_unique<camera>(*app, true, fov);
    cam->set_position({ 8.f, 16.f, 8.f });
    cam->look_at({ 0.f, 0.f, 0.f });

    int2 fbo_resolutions[] = {
        { 120, 90 },
        { 320, 180 },
        { 640, 480 },
        { 800, 600 },
        { 1280, 720 },
    };
    const char* fbo_resolution_names[] = { "120 x 90", "320 x 180", "640 x 480", "800 x 600", "1280 x 720" };
    int fbo_resolution = 4;
    int total_fbo_res = sizeof(fbo_resolutions) / sizeof(fbo_resolutions[0]);

    auto reload_fbos = [&]() {
        go->background_pass = std::make_shared<fbo>(
            fbo_resolutions[fbo_resolution].x, 
            fbo_resolutions[fbo_resolution].y);
        go->background_pass->createTextureAttachment(textures.get(first_pass_color));
        go->background_pass->createDepthBufferAttachment();

        go->tubes_interior_pass = std::make_shared<fbo>(
            fbo_resolutions[fbo_resolution].x,
            fbo_resolutions[fbo_resolution].y);
        go->tubes_interior_pass->createTextureAttachment(textures.get(second_pass_color));
        go->tubes_interior_pass->createDepthBufferAttachment();
    };

    std::vector<const char*> tubes_names;
    std::vector<std::shared_ptr<vao>> tube_vaos;
    std::vector<obj_mesh*> tube_meshes;
    std::map<std::string, obj_mesh> tube_types;
    int tube_idx = 0;

    {
        auto cylinder = generate_tube(length, radius, 0.f, a, b);
        auto bent_cylinder = generate_tube(length, radius, -1.f, a, b);
        auto cap = generate_cap(1.f, radius, 0.5f);
        float3x3 r{
            { 0.f, 0.f, 1.f },
            { 0.f, 1.f, 0.f },
            { -1.f, 0.f, 0.f }
        };
        float3x3 id{
            { 1.f, 0.f, 0.f },
            { 0.f, 1.f, 0.f },
            { 0.f, 0.f, 1.f }
        };

        auto t2x2h = generate_tube(2.f, 1.f, 0, 1, 32);
        auto t2x2l = generate_tube(2.f, 1.f, 0, 1, 12);
        tube_types["2x2,1,H"] = apply(
            t2x2h, id, { 0.f, 0.f, 0.5f });
        tube_types["2x2,1,L"] = apply(
            t2x2l, id, { 0.f, 0.f, 0.5f });

        tube_types["2x2,2,H"] = apply(
            t2x2h, r, { 0.0f, 0.f, 0.5f });
        tube_types["2x2,2,L"] = apply(
            t2x2l, r, { 0.f, 0.f, 0.5f });

        auto v2x3h = generate_tube(3.f, 1.f, 0, 1, 32);
        auto v2x3l = generate_tube(3.f, 1.f, 0, 1, 12);
        tube_types["2x3,1,H"] = v2x3h;
        tube_types["2x3,1,L"] = v2x3l;

        tube_types["2x3,2,H"] = apply(
            v2x3h, r, { 0.5f, 0.f, 0.5f });
        tube_types["2x3,2,L"] = apply(
            v2x3l, r, { 0.5f, 0.f, 0.5f }, true);

        tube_types["2x3,3,H"] = apply(
            v2x3h, id, { 0.f, 0.f, 1.f });
        tube_types["2x3,3,L"] = apply(
            v2x3l, id, { 0.f, 0.f, 1.f }, true);

        tube_types["2x3,4,H"] = apply(
            v2x3h, r, { -0.5f, 0.f, 0.5f });
        tube_types["2x3,4,L"] = apply(
            v2x3l, r, { -0.5f, 0.f, 0.5f }, true);

        auto t3x3h = generate_tube(3.f, 1.f, -1.f, 32, 32);
        auto t3x3l = generate_tube(3.f, 1.f, -1.f, 12, 12);
        tube_types["3x3,1,H"] = apply(
            t3x3h, id, { 0.f, 0.f, 0.f });
        tube_types["3x3,2,H"] = apply(
            t3x3h, r, { 0.5f, 0.f, 0.5f });
        tube_types["3x3,3,H"] = apply(
            t3x3h, mul(r, r), { 0.f, 0.f, 1.f });
        tube_types["3x3,4,H"] = apply(
            t3x3h, mul(r, mul(r, r)), { -0.5f, 0.f, 0.5f });

        tube_types["3x3,1,L"] = apply(
            t3x3l, id, { 0.f, 0.f, 0.f });
        tube_types["3x3,2,L"] = apply(
            t3x3l, r, { 0.5f, 0.f, 0.5f });
        tube_types["3x3,3,L"] = apply(
            t3x3l, mul(r, r), { 0.f, 0.f, 1.f });
        tube_types["3x3,4,L"] = apply(
            t3x3l, mul(r, mul(r, r)), { -0.5f, 0.f, 0.5f });

        auto t4x3h = generate_tube3(length, radius, 16, 16);
        auto t4x3l = generate_tube3(length, radius, 16, 16);
        tube_types["4x3,1,H"] = apply(
            t4x3h, id, { 0.f, 0.f, 1.f });
        tube_types["4x3,1,L"] = apply(
            t4x3l, id, { 0.f, 0.f, 1.f });

        tube_types["4x3,2,H"] = apply(
            t4x3h, r, { -0.5f, 0.f, 0.5f });
        tube_types["4x3,2,L"] = apply(
            t4x3l, r, { -0.5f, 0.f, 0.5f });

        tube_types["4x3,3,H"] = apply(
            t4x3h, mul(r, r), { 0.f, 0.f, 0.f });
        tube_types["4x3,3,L"] = apply(
            t4x3l, mul(r, r), { 0.f, 0.f, 0.f });

        tube_types["4x3,4,H"] = apply(
            t4x3h, mul(r, mul(r, r)), { 0.5f, 0.f, 0.5f });
        tube_types["4x3,4,L"] = apply(
            t4x3l, mul(r, mul(r, r)), { 0.5f, 0.f, 0.5f });

        auto t4x4h = generate_tube4(length, radius, -1.f, 16, 16);
        auto t4x4l = generate_tube4(length, radius, -1.f, 16, 16);
        tube_types["4x4,1,H"] = apply(
            t4x4h, id, { 0.f, 0.f, 1.f });
        tube_types["4x4,1,L"] = apply(
            t4x4l, id, { 0.f, 0.f, 1.f });
    }

    auto reload_models = [&]() {
        tube_vaos.clear();
        tubes_names.clear();
        tube_meshes.clear();
        int counter = 0;
        for (auto& kvp : tube_types)
        {
            if (counter++ % 2)
                tubes_names.push_back(kvp.first.c_str());
            tube_vaos.push_back(vao::create(kvp.second));
            tube_meshes.push_back(&kvp.second);
        }
        tube_idx = tubes_names.size() - 1;

        app->is_alive();

        glass.generate_decals(white);
        app->is_alive();

        go->grid = vao::create(make_grid(20, 20, 1.f, 1.f));
        go->cat = vao::create(cat_ld.get().front());
        go->earth = vao::create(ld.get().front());
    };

    auto reload_graphics = [&]() {
        go = std::make_shared<graphic_objects>();

        reload_textures();
        reload_fbos();
        reload_models();
    };
    reload_graphics();

    float4x4 matrix;

    auto draw_stuff_inside = [&]()
    {
        go->shader.set_model(mul(
            translation_matrix(float3{ 0.f, -0.6f, -2.f }),
            scaling_matrix(float3{ 1.5f, 1.5f, 1.5f })
        ));

        textures.with_texture(cat_tex, 0, [&]() {
            go->cat->draw();
        });
    };

    bool show_grid = false;

    auto draw_refractables = [&](float t) {
        double factor = sin(t / 2.0);
        double x = -0.2 * factor;
        double y = -0.8 * factor;
        double z = 0 * factor;
        double w = cos(t / 2.0);
        float4 q(x, y, z, w);
        q = normalize(q);

        go->shader.set_model(mul(
            translation_matrix(float3{ 0.f, -9.f, -5.f }),
            scaling_matrix(float3{ 2.f, 2.f, 2.f }),
            rotation_matrix(q)
        ));

        textures.with_texture(world, 0, [&]() {
            go->earth->draw();
        });

        go->shader.set_model(mul(
            translation_matrix(float3{ 0.f, -1.f, 0.5f }),
            scaling_matrix(float3{ 1.f, 1.f, 1.f })
        ));

        if (show_grid)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            textures.with_texture(white, 0, [&]() {
                go->grid->draw();
            });
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    };

    float last_t = 1.f;
    float4x4 tsp{
        { 1.f, 0.f, 0.f, 0.f },
        { 0.f, 1.f, 0.f, 0.f },
        { 0.f, 0.f, 1.f, 0.f },
        { 0.f, 0.f, 0.f, 1.f }
    };

    float3 pos;
    bool build_tool_active = false;

    auto draw_tubes = [&](texture_handle refraction_id, int tube_type) {
        textures.with_texture(mish, go->tb_shader.diffuse_slot(), [&]() {
        textures.with_texture(normals, go->tb_shader.normal_map_slot(), [&]() {
        textures.with_texture(refraction_id, go->tb_shader.refraction_slot(), [&]() {
            go->tb_shader.enable_normal_mapping(true);

            if (build_tool_active)
            {
                go->tb_shader.set_model(mul(
                    translation_matrix(pos),
                    scaling_matrix(float3{ 1.f, 1.f, 1.f })
                ));
                tube_vaos[tube_type * 2]->draw();
            }

            //go->tb_shader.set_model(mul(
            //    translation_matrix(float3{ 0.f, 0.f, 5.f }),
            //    scaling_matrix(float3{ 1.f, 1.f, 1.f })
            //));
            ////glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            //go->cap->draw();
            ////glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            //go->tb_shader.set_model(mul(
            //    translation_matrix(float3{ 0.f, 0.f, 3.f }),
            //    scaling_matrix(float3{ 1.f, 1.f, 1.f })
            //));
            //go->tube->draw();

            for (auto& t : tubes)
            {
                auto trans = mul(
                    translation_matrix(float3{ (float)t.pos.x, 0.f, (float)t.pos.y }),
                    scaling_matrix(float3{ 1.f, 1.f, 1.f })
                );
                go->tb_shader.set_model(trans);
                if (t.damaged)
                {
                    glass.set_hitpoint(*tube_meshes[t.type * 2], trans, t.damaged_idx, t.glass_decal);
                    glass.prepare_decal(go->tb_shader);
                    textures.with_texture(glass.diffuse(), go->tb_shader.glass_atlas_slot(), [&]() {
                    textures.with_texture(glass.outline(), go->tb_shader.decal_atlas_slot(), [&]() {
                        tube_vaos[t.type * 2]->draw();
                    });
                    });
                }
                else
                {
                    textures.with_texture(empty_texture, go->tb_shader.glass_atlas_slot(), [&]() {
                    textures.with_texture(empty_texture, go->tb_shader.decal_atlas_slot(), [&]() {
                        tube_vaos[t.type * 2]->draw();
                    });
                    });
                }
            }

            /*go->tb_shader.set_model(mul(
                translation_matrix(float3{ 0.f, 0.f, -3.f }),
                scaling_matrix(float3{ 1.f, 1.f, 1.f })
            ));

            textures.with_texture(glass.diffuse(), go->tb_shader.glass_atlas_slot(), [&]() {
            textures.with_texture(glass.outline(), go->tb_shader.decal_atlas_slot(), [&]() {
                go->bent_tube->draw();
            });
            });*/
        });
        });
        });
    };

    auto draw_scatter = [&](texture_handle refraction_id, float t) {
        go->tb_shader.set_distortion(0.f);
        go->tb_shader.set_material_properties(diffuse_level, shineDamper, reflectivity);

        textures.with_texture(mish, go->tb_shader.diffuse_slot(), [&]() {
        textures.with_texture(normals, go->tb_shader.normal_map_slot(), [&]() {
        textures.with_texture(refraction_id, go->tb_shader.refraction_slot(), [&]() {
            for (auto& tb : tubes)
            {
                if (tb.damaged)
                {
                    auto trans = mul(
                        translation_matrix(float3{ (float)tb.pos.x, 0.f, (float)tb.pos.y }),
                        scaling_matrix(float3{ 1.f, 1.f, 1.f })
                    );
                    glass.set_hitpoint(*tube_meshes[tb.type * 2], trans, tb.damaged_idx, tb.glass_decal);
                    
                    glass.draw_scatter(t, go->tb_shader);
                }
            }
        });
        });
        });

        go->tb_shader.enable_normal_mapping(true);
    };

    bool fullscreen = false;
    bool msaa = true;
    int multisample = 4;
    int2 resolutions[] = {
        { 1280, 720 },
        { 1920, 1080 }
    };
    const char* resolution_names[] = { "1280 x 720", "1920 x 1080" };
    int resolution = 0;
    int total_resolutions = sizeof(resolutions) / sizeof(resolutions[0]);

    int2 tex_settings[] = {
        { 0, 0 }, { 1, 0 }, { 0, 1 }
    };
    int tex_setting = 2;
    const char* tex_setting_names[] = { "Nearest", "Linear", "Mipmap" };
    int total_tex_settings = sizeof(tex_settings) / sizeof(tex_settings[0]);

    bool requires_reset = false;
    bool do_reset = false;
    bool exit = false;

    cam->update(*app, true);

    LOG(INFO) << "Checking for errors during Initialization";
    app->is_alive();
    LOG(INFO) << "All Good, starting main loop";

    int tube_type = 0;

    if (file_exists("resources/save.dat"))
    {
        load_tubes(tubes);
    }

    while (app->is_alive() && !exit)
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        //if (app->get_mouse().mouse_wheel && app->get_mouse().x >= 250)
        //{
        //    fov = std::min(500.f, std::max(50.f, fov + app->get_mouse().mouse_wheel * 10));

        //    auto pos = cam->get_position();
        //    auto target = cam->get_target();

        //    cam = std::make_unique<camera>(*app, false, fov);

        //    cam->set_position(pos);
        //    cam->look_at(target);
        //    cam->update(*app, true);
        //}

        if (build_tool_active && app->get_mouse().x > 250)
        {
            if (ImGui::IsMouseClicked(1)) tube_type = (tube_type + 1) % (tube_vaos.size() / 2);
            if (ImGui::IsMouseClicked(0))
            {
                tube_peice tb;
                tb.pos = { (int)pos.x, (int)pos.z };
                tb.type = tube_type;
                tubes.push_back(tb);
            }
        }

        auto s = std::abs(std::sinf(app->get_time())) * 0.2f + 0.8f;
        auto t = std::abs(std::sinf(app->get_time() + 5)) * 0.2f + 0.8f;

        if (rotate_light)
        {
            light_angle = app->get_time();
            while (light_angle > 2 * 3.14) light_angle -= 2 * 3.14;
        }

        l.position = { 5.f * std::sinf(-light_angle), 1.5f, 5.f * std::cosf(-light_angle) };
        l.colour = { s, t, s };

        t = app->get_time() / 10;

        cam->update(*app);

        go->shader.begin();

        go->shader.set_material_properties(diffuse_level, shineDamper, reflectivity);
        go->shader.set_light(l.position);
        go->shader.set_mvp(matrix, cam->view_matrix(), cam->projection_matrix());

        go->background_pass->bind();
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        draw_refractables(t);

        go->background_pass->unbind();

        go->tubes_interior_pass->bind();
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        go->viz.draw_texture(textures.get(first_pass_color));

        glCullFace(GL_FRONT);

        go->shader.end();
        go->tb_shader.begin();

        go->tb_shader.set_material_properties(diffuse_level, shineDamper, reflectivity);
        go->tb_shader.set_light(l.position);
        go->tb_shader.set_mvp(matrix, cam->view_matrix(), cam->projection_matrix());
        go->tb_shader.set_distortion(0.2f);

        glDisable(GL_DEPTH_TEST);
        draw_tubes(first_pass_color, tube_type);
        glEnable(GL_DEPTH_TEST);

        glCullFace(GL_BACK);

        go->tb_shader.end();
        go->shader.begin();

        draw_stuff_inside();

        go->shader.end();
        go->tb_shader.begin();

        go->tubes_interior_pass->unbind();

        app->reset_viewport();
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        draw_tubes(first_pass_color, tube_type);

        //--
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        go->tb_shader.set_distortion(0.1f);
        
        draw_tubes(second_pass_color, tube_type);
        draw_scatter(second_pass_color, t);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        go->tb_shader.end();
        go->shader.begin();

        draw_refractables(t);

        go->shader.end();

        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        auto ray = mouse_pick(
            int2{ (int)app->get_mouse().x, (int)app->get_mouse().y },
            int2{ app->width(), app->height() },
            cam->projection_matrix(),
            cam->view_matrix());
        //LOG(INFO) << ray.x << " " << ray.y << " " << ray.z;

        auto normal = float3{ 0.f,1.f,0.f };

        auto t1 = -cam->get_position().y / ray.y;

        pos = cam->get_position() + t1 * ray;
        pos.x = floorf(pos.x);
        pos.z = floorf(pos.z);
        //LOG(INFO) << t1 << ":\t" <<  ray.x << " " << ray.y << " " << ray.z;

        const auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse;
        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ 250, static_cast<float>(app->height()) });

        ImGui::Begin("Control Panel", nullptr, flags);

        //std::stringstream ss;
        //ss << std::setprecision(3) << ray.x << " " << ray.y << " " << ray.z;
        //ImGui::Text("%s", ss.str().c_str());
        //ss.str("");
        //ss << t1 << ": " << intr.x << " " << intr.y << " " << intr.z;
        //ImGui::Text("%s", ss.str().c_str());

        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

        if (ImGui::Button("Exit to Desktop", { 235, 0 }))
            exit = true;

        if (ImGui::CollapsingHeader("Controls", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Button("Randomize"))
            {
                int random_tube = rand() % tubes.size();
                auto& t = tubes[random_tube];
                t.damaged = true;
                t.damaged_idx = rand() % tube_meshes[t.type * 2]->positions.size();
                t.glass_decal = rand();
            }

            if (ImCheckButton(&build_tool_active, "Exit Build Mode", "Enter Build Mode"))
            {
                tube_type = 0;
            }

            if (ImGui::Button("Clear", { 70, 0 })) tubes.clear();
            ImGui::SameLine();
            if (ImGui::Button("Save", { 75, 0 })) save_tubes(tubes);

            ImGui::SameLine();
            if (ImGui::Button("Load", { 75, 0 })) load_tubes(tubes);

            ImGui::Checkbox("Display Grid", &show_grid);
        }

        if (ImGui::CollapsingHeader("Light Settings"))
        {
            ImGui::Text("Specular Dampening:");
            ImGui::PushItemWidth(-1);
            ImGui::SliderFloat("##SpecDamp", &shineDamper, 0.f, 100.f);

            ImGui::Text("Reflectivity:");
            ImGui::PushItemWidth(-1);
            ImGui::SliderFloat("##Reflect", &reflectivity, 0.f, 3.f);

            ImGui::Text("Diffuse:");
            ImGui::PushItemWidth(-1);
            ImGui::SliderFloat("##Diffuse", &diffuse_level, 0.f, 1.f);

            ImGui::Checkbox("Rotate Light", &rotate_light);
            ImGui::PushItemWidth(-1);
            ImGui::SliderFloat("##Rotation", &light_angle, 0.f, 2 * 3.14f);
        }

        if (ImGui::CollapsingHeader("Texture Settings"))
        {
            if (ImGui::Combo("Filtering", &tex_setting,
                tex_setting_names, total_tex_settings))
            {
                linear = tex_settings[tex_setting].x;
                mipmap = tex_settings[tex_setting].y;
                reload_textures();
            }

            if (ImGui::Combo("Refraction Resolution", &fbo_resolution,
                fbo_resolution_names, total_fbo_res))
            {
                reload_fbos();
            }
        }

        if (ImGui::CollapsingHeader("Model Settings"))
        {
            if (ImGui::Combo("Model Type", &tube_idx,
                tubes_names.data(), tubes_names.size()))
            {
                //reload_models();
            }
        }

        if (ImGui::CollapsingHeader("Camera Settings"))
        {
            auto pos = cam->get_position();
            auto target = cam->get_target();

            if (ImGui::RadioButton("Perspective Projection", cam->is_perspective()))
            {
                cam = std::make_unique<camera>(*app, true, fov);
            }

            if (ImGui::RadioButton("Orthographic Projection", !cam->is_perspective()))
            {
                cam = std::make_unique<camera>(*app, false, fov);
            }

            if (cam->is_perspective())
            {
                ImGui::Text("FOV:");
                ImGui::PushItemWidth(-1);
                if (ImGui::SliderFloat("##FOV", &fov, 60.f, 180.f))
                {
                    cam = std::make_unique<camera>(*app, true, fov);
                }
            }
            else
            {
                ImGui::Text("Zoom:");
                ImGui::PushItemWidth(-1);
                if (ImGui::SliderFloat("##Zoom", &fov, 60.f, 180.f))
                {
                    cam = std::make_unique<camera>(*app, false, fov);
                }
            }

            cam->set_position(pos);
            cam->look_at(target);
            cam->update(*app, true);
        }

        if (ImGui::CollapsingHeader("Window Settings"))
        {
            if (ImGui::Combo("Resolution", &resolution, 
                resolution_names, total_resolutions)) 
                requires_reset = true;

            if (ImGui::Checkbox("Fullscreen", &fullscreen)) 
                requires_reset = true;

            if (ImGui::Checkbox("MSAA", &msaa)) 
                requires_reset = true;

            if (msaa)
            {
                ImGui::Text("Samples:");
                ImGui::PushItemWidth(-1);
                if (ImGui::SliderInt("##Samples", &multisample, 1, 16)) 
                    requires_reset = true;
            }

            if (requires_reset)
            {
                if (ImGui::Button(" Apply "))
                {
                    do_reset = true;
                }
                ImGui::SameLine();
                if (ImGui::Button(" Undo "))
                {
                    msaa = app->multisample() > 0;
                    if (msaa) multisample = app->multisample();
                    for (int i = 0; i < total_resolutions; i++)
                    {
                        if (resolutions[i].x == app->width() &&
                            resolutions[i].y == app->height())
                            resolution = i;
                    }
                    fullscreen = app->fullscreen();
                    requires_reset = false;
                }
            }
        }

        std::vector<std::function<void()>> later;

        if (ImGui::CollapsingHeader("Textures"))
        {
            static int selected_texture = 0;
            std::vector<const char*> texture_names;
            long total_bytes = 0;
            float load_time = 0.f;
            for (auto& to : textures.get_texture_objects())
            {
                total_bytes += to.get().get_bytes();
                load_time += to.get().get_load_time();
                texture_names.push_back(to.get_name().c_str());
            }
            auto bytes_str = bytes_to_string(total_bytes);
            ImGui::Text("Total Texture Memory: %s", bytes_str.c_str());
            ImGui::Text("Load Time: %f ms", load_time);

            auto& to = textures.get_texture_objects()[selected_texture];
            {
                std::stringstream ss;
                auto cur = ImGui::GetCursorScreenPos();

                later.push_back([cur, &app, &to, go]() {
                    float2 pos{ ((cur.x + 110.f) / app->width()) * 2.f - 1.f,
                        1.f - ((cur.y + 110.f) / app->height()) * 2.f };
                    float2 scale{ 220.f / app->width(), 220.f / app->height() };

                    go->viz.draw_texture(pos, scale, to.get());
                });

                ImGui::SetCursorScreenPos(ImVec2(cur.x, cur.y + 220.f));

                ss.str("");
                ss << "Open / Close##" << to.get_name();

                if (to.is_open())
                {
                    if (ImGui::Button(ss.str().c_str(), ImVec2{ 220.f, 0.f })) to.is_open() = false;
                }
                else
                {
                    if (ImGui::Button(ss.str().c_str(), ImVec2{ 220.f, 0.f })) to.is_open() = true;
                }

                bool reload = false;

                ss.str("");
                ss << to.get().get_width() << " x " << to.get().get_height() << " px; ";
                ImGui::Text("%s", ss.str().c_str());
                ImGui::SameLine();
                auto bytes_str = bytes_to_string(to.get().get_bytes());
                ImGui::Text("%s", bytes_str.c_str());

                ss.str("");
                ss << "Linear Filtering##" << to.get_name();
                if (ImGui::Checkbox(ss.str().c_str(), &to.linear())) reload = true;

                ss.str("");
                ss << "Mipmap##" << to.get_name();
                if (ImGui::Checkbox(ss.str().c_str(), &to.mipmap())) reload = true;

                if (reload)
                    to.reload();
            }

            ImGui::Combo("##Texture", &selected_texture,
                texture_names.data(), texture_names.size());
        }

        ImGui::End();

        for (auto& to : textures.get_texture_objects())
        {
            if (to.is_open())
            {
                ImGui::SetNextWindowSize({ 
                    to.get().get_width() + 20.f, 
                    to.get().get_height() + 20.f });

                std::stringstream ss;
                ss << to.get_name() << "##Texture_Window";
                if (ImGui::Begin(ss.str().c_str()))
                {
                    auto cur = ImGui::GetCursorScreenPos();

                    later.push_back([cur, &app, &to, go]() {
                        float2 pos{ ((cur.x + to.get().get_width() / 2.f) / app->width()) * 2.f - 1.f,
                            1.f - ((cur.y + to.get().get_height() / 2.f) / app->height()) * 2.f };
                        float2 scale{ to.get().get_width() / (float)app->width(), 
                                      to.get().get_height() / (float)app->height() };

                        go->viz.draw_texture(pos, scale, to.get());
                    });
                }

                ImGui::End();
            }
        }

        app->end_ui();
        for (auto& a : later) a();

        if (do_reset)
        {
            LOG(INFO) << "Releasing all OpenGL objects and Window";
            go.reset();
            glass.release();
            for (auto& to : textures.get_texture_objects()) to.release();
            app.reset();

            LOG(INFO) << "Recreating the Window";
            app = std::make_unique<window>(
                resolutions[resolution].x, 
                resolutions[resolution].y, 
                "Shader Playground",
                msaa ? multisample : 0, 
                fullscreen);

            LOG(INFO) << "Recreating all OpenGL objects";
            reload_graphics();

            do_reset = false;
            requires_reset = false;
        }
    }

    return EXIT_SUCCESS;
}