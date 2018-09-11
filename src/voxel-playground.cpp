#include <iostream>
#include <chrono>

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

#include <easylogging++.h>
INITIALIZE_EASYLOGGINGPP

#include <imgui.h>

#include <math.h>
#include <map>

#define LOG_GL_INT(x) { int max_textures = 0;\
glGetIntegerv(x, &max_textures);\
LOG(INFO) << #x << " = " << max_textures;\
}

class visualizer_2d
{
public:
    visualizer_2d() {}

    void draw_texture(float2 pos, float2 scale, texture& tex)
    {
        tex_2d_shader.begin();
        _visualizer.set_position(pos);
        _visualizer.set_scale(scale);
        _visualizer.draw(tex_2d_shader, tex);
        tex_2d_shader.end();
    }
private:
    texture_visualizer _visualizer;
    texture_2d_shader tex_2d_shader;
};

class texture_object
{
public:
    texture_object(std::string name, 
        std::string filename)
        : _name(std::move(name)), _filename(std::move(filename))
    {
        _tex = std::make_shared<texture>();
    }

    texture& get() { return *_tex; }

    const std::string& get_name() const { return _name; }

    void release() { _tex.reset(); }

    void reload()
    {
        _tex = std::make_shared<texture>();
        if (_filename != "")
        {
            _tex->set_options(_linear, _mipmap);
            _tex->upload(_filename);
        }
    }

    bool& linear() { return _linear; }
    bool& mipmap() { return _mipmap; }
    bool& is_open() { return _is_open; }
private:
    std::shared_ptr<texture> _tex;
    std::string _name, _filename = "";

    bool _linear = true, _mipmap = true, _is_open = false;
};

int add_texture(std::vector<texture_object>& tos, std::string name, std::string filename)
{
    tos.emplace_back(name, filename);
    return tos.size() - 1;
}

template<class T>
void with_texture(std::vector<texture_object>& tos, int tex_id, int slot, T action)
{
    auto& tex = tos[tex_id].get();
    tex.bind(slot);
    action();
    tex.unbind();
}

struct graphic_objects
{
    std::map<std::string, std::shared_ptr<vao>> tubes;

    std::shared_ptr<vao> earth, tube, bent_tube, 
        rotated_tube, cat, grid, cap;

    visualizer_2d viz;

    std::vector<std::vector<std::pair<std::shared_ptr<vao>, glass_peice>>> glasses;
    int glass_id = 0;

    simple_shader shader;
    tube_shader tb_shader;
    texture_2d_shader tex_2d_shader;
    glass_decals_shader blur_shader;

    std::shared_ptr<fbo> fbo1, fbo2;
    std::shared_ptr<fbo> glass_impact, glass_impact2;
};

int main(int argc, char* argv[])
{
    srand(time(NULL));

    START_EASYLOGGINGPP(argc, argv);

    auto app = std::make_unique<window>(1280, 720, "Shader Playground");

    float progress = 0.f;

    auto go = std::make_shared<graphic_objects>();

    bool mipmap = true;
    bool linear = true;

    std::vector<texture_object> textures;
    int mish = add_texture(textures, "diffuse", "resources/texture.png");
    int normals = add_texture(textures, "normal_map", "resources/normal_map.png");
    int world = add_texture(textures, "world diffuse", "resources/Diffuse_2K.png");
    int cat_tex = add_texture(textures, "cat diffuse", "resources/cat_diff.tga");
    int white = add_texture(textures, "white", "resources/white.png");

    int first_pass_color = add_texture(textures, "first_pass_color", "");
    int second_pass_color = add_texture(textures, "second_pass_color", "");
    int temp_glass_atlas = add_texture(textures, "temp_glass_atlas", "");
    int final_glass_atlas = add_texture(textures, "final_glass_atlas", "");

    auto reload_textures = [&]() {
        for (auto& to : textures)
            to.reload();
    };

    /*auto t = 1.6f;
    float3x3 m {
        { cosf(t), 0, sinf(t) },
        { 0, 1, 0 },
        { -sinf(t), 0, cosf(t) }
    };

    auto r = apply(cylinder, m);

    std::vector<int> edge1, edge2;
    r = filter(r, [](const float3& p) {
        return sqrt(p.x * p.x + p.y * p.y) > 1.1f || fabsf(p.y) > 0.9f;
    }, edge1);
    cylinder = filter(cylinder, [](const float3& p) {
        return sqrt(p.y * p.y + p.z * p.z) > 1.1f;
    }, edge2);

    for (auto& e : edge2) e += r.positions.size();

    cylinder = fuse(r, cylinder);*/
    //int i = edge1.back();
    //edge1.pop_back();
    //int j = nearest(cylinder, edge2, i);
    //int k = nearest(cylinder, edge1, i);

    //cylinder.indexes.emplace_back(i, j, k);

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
    float fov = 160.f;

    int a = 27;
    int b = 27;
    float radius = 1.f;
    float length = 3.f;

    auto cam = std::make_unique<camera>(*app, false, fov);
    cam->set_position({ 50.f, 50.f, 50.f });
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
        go->fbo1 = std::make_shared<fbo>(
            fbo_resolutions[fbo_resolution].x, 
            fbo_resolutions[fbo_resolution].y);
        go->fbo1->createTextureAttachment(textures[first_pass_color].get());
        go->fbo1->createDepthBufferAttachment();

        go->fbo2 = std::make_shared<fbo>(
            fbo_resolutions[fbo_resolution].x,
            fbo_resolutions[fbo_resolution].y);
        go->fbo2->createTextureAttachment(textures[second_pass_color].get());
        go->fbo2->createDepthBufferAttachment();

        go->glass_impact = std::make_shared<fbo>(1024, 1024);
        go->glass_impact->createTextureAttachment(textures[final_glass_atlas].get());

        go->glass_impact2 = std::make_shared<fbo>(1024, 1024);
        go->glass_impact2->createTextureAttachment(textures[temp_glass_atlas].get());
    };

    std::vector<const char*> tubes_names;
    std::vector<vao*> tube_vaos;
    int tube_idx = 0;

    auto cylinder = generate_tube(length, radius, 0.f, a, b);
    auto bent_cylinder = generate_tube(length, radius, -1.f, a, b);
    auto cap = generate_cap(1.f, radius, 0.5f);

    const int glass_variations = 4;
    std::vector<std::vector<glass_peice>> glass_models;
    for (int i = 0; i < glass_variations*glass_variations; i++)
    {
        std::vector<glass_peice> t;
        generate_broken_glass(t);
        glass_models.emplace_back(t);
    }

    auto reload_models = [&]() {

        go->tubes.clear();

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
        go->tubes["2x2,1,H"] = vao::create(apply(
            t2x2h, id, { 0.f, 0.f, 0.5f }));
        go->tubes["2x2,1,L"] = vao::create(apply(
            t2x2l, id, { 0.f, 0.f, 0.5f }));

        go->tubes["2x2,2,H"] = vao::create(apply(
            t2x2h, r, { 0.0f, 0.f, 0.5f }));
        go->tubes["2x2,2,L"] = vao::create(apply(
            t2x2l, r, { 0.f, 0.f, 0.5f }));

        auto v2x3h = generate_tube(3.f, 1.f, 0, 1, 32);
        auto v2x3l = generate_tube(3.f, 1.f, 0, 1, 12);
        go->tubes["2x3,1,H"] = vao::create(v2x3h);
        go->tubes["2x3,1,L"] = vao::create(v2x3l);

        go->tubes["2x3,2,H"] = vao::create(apply(
            v2x3h, r, { 0.5f, 0.f, 0.5f }));
        go->tubes["2x3,2,L"] = vao::create(apply(
            v2x3l, r, { 0.5f, 0.f, 0.5f }, true));

        go->tubes["2x3,3,H"] = vao::create(apply(
            v2x3h, id, { 0.f, 0.f, 1.f }));
        go->tubes["2x3,3,L"] = vao::create(apply(
            v2x3l, id, { 0.f, 0.f, 1.f }, true));

        go->tubes["2x3,4,H"] = vao::create(apply(
            v2x3h, r, { -0.5f, 0.f, 0.5f }));
        go->tubes["2x3,4,L"] = vao::create(apply(
            v2x3l, r, { -0.5f, 0.f, 0.5f }, true));

        auto t3x3h = generate_tube(3.f, 1.f, -1.f, 32, 32);
        auto t3x3l = generate_tube(3.f, 1.f, -1.f, 12, 12);
        go->tubes["3x3,1,H"] = vao::create(apply(
            t3x3h, id, { 0.f, 0.f, 0.f }));
        go->tubes["3x3,2,H"] = vao::create(apply(
            t3x3h, r, { 0.5f, 0.f, 0.5f }));
        go->tubes["3x3,3,H"] = vao::create(apply(
            t3x3h, mul(r,r), { 0.f, 0.f, 1.f }));
        go->tubes["3x3,4,H"] = vao::create(apply(
            t3x3h, mul(r,mul(r, r)), { -0.5f, 0.f, 0.5f }));

        go->tubes["3x3,1,L"] = vao::create(apply(
            t3x3l, id, { 0.f, 0.f, 0.f }));
        go->tubes["3x3,2,L"] = vao::create(apply(
            t3x3l, r, { 0.5f, 0.f, 0.5f }));
        go->tubes["3x3,3,L"] = vao::create(apply(
            t3x3l, mul(r, r), { 0.f, 0.f, 1.f }));
        go->tubes["3x3,4,L"] = vao::create(apply(
            t3x3l, mul(r, mul(r, r)), { -0.5f, 0.f, 0.5f }));

        auto t4x3h = generate_tube3(length, radius, 16, 16);
        auto t4x3l = generate_tube3(length, radius, 16, 16);
        go->tubes["4x3,1,H"] = vao::create(apply(
            t4x3h, id, { 0.f, 0.f, 1.f }));
        go->tubes["4x3,1,L"] = vao::create(apply(
            t4x3l, id, { 0.f, 0.f, 1.f }));

        go->tubes["4x3,2,H"] = vao::create(apply(
            t4x3h, r, { -0.5f, 0.f, 0.5f }));
        go->tubes["4x3,2,L"] = vao::create(apply(
            t4x3l, r, { -0.5f, 0.f, 0.5f }));

        go->tubes["4x3,3,H"] = vao::create(apply(
            t4x3h, mul(r, r), { 0.f, 0.f, 0.f }));
        go->tubes["4x3,3,L"] = vao::create(apply(
            t4x3l, mul(r, r), { 0.f, 0.f, 0.f }));

        go->tubes["4x3,4,H"] = vao::create(apply(
            t4x3h, mul(r, mul(r, r)), { 0.5f, 0.f, 0.5f }));
        go->tubes["4x3,4,L"] = vao::create(apply(
            t4x3l, mul(r, mul(r, r)), { 0.5f, 0.f, 0.5f }));

        auto t4x4h = generate_tube4(length, radius, -1.f, 16, 16);
        auto t4x4l = generate_tube4(length, radius, -1.f, 16, 16);
        go->tubes["4x4,1,H"] = vao::create(apply(
            t4x4h, id, { 0.f, 0.f, 1.f }));
        go->tubes["4x4,1,L"] = vao::create(apply(
            t4x4l, id, { 0.f, 0.f, 1.f }));

        tube_vaos.clear();
        tubes_names.clear();
        int counter = 0;
        for (auto& kvp : go->tubes)
        {
            if (counter++ % 2)
                tubes_names.push_back(kvp.first.c_str());
            tube_vaos.push_back(kvp.second.get());
        }
        tube_idx = tubes_names.size() - 1;

        LOG(INFO) << "Generating Glass Decals:";
        go->glasses.clear();

        go->shader.begin();
        go->glass_impact->bind();
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        go->shader.set_light({ 0.f, 0.f, -1.f });
        go->shader.set_material_properties(1.f, 1.f, 0.f);

        auto projection = create_orthographic_projection_matrix(
            go->glass_impact->get_width(),
            go->glass_impact->get_height(), go->glass_impact->get_width(), 0.1f, 1000.f);
        go->shader.set_mvp(identity_t{}, 
            scaling_matrix(float3{ 2.f / glass_variations, 2.f / glass_variations, -0.5f }), 
            projection);

        with_texture(textures, white, 0, [&]() {
            int index = 0;
            for (auto& gm : glass_models)
            {
                std::vector<std::pair<std::shared_ptr<vao>, glass_peice>> glass;
                for (auto& p : gm)
                {
                    glass.emplace_back(
                        vao::create(p.peice),
                        p
                    );
                }

                for (auto& g : glass)
                {
                    auto view =
                        translation_matrix(float3{
                        g.second.pos.x,
                        g.second.pos.y,
                        0.f });

                    int i = index / glass_variations - glass_variations / 2;
                    int j = index % glass_variations - glass_variations / 2;

                    view = mul(
                        translation_matrix(float3{ i + 0.1f, j + 1.f - 0.1f, 1.f }),
                        scaling_matrix(float3{ 0.8f, 0.8f, 1.f }), // to prevent any pixels from sticking to the edge
                        view);

                    go->shader.set_model(view);

                    g.first->draw();
                }

                go->glasses.emplace_back(glass);

                index++;
            }
        });

        go->glass_impact->unbind();
        go->shader.end();

        LOG(INFO) << "Applying horizontal blur:";
        go->glass_impact2->bind();
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
        go->blur_shader.begin();
        {
            texture_visualizer fbo_visualizer(float2{ 0.0f, 0.0f }, float2{ 1.f, 1.f });
            go->blur_shader.set_width_height(true, go->glass_impact->get_width(), go->glass_impact->get_height());
            fbo_visualizer.draw(go->blur_shader, textures[final_glass_atlas].get());
        }

        go->blur_shader.end();
        go->glass_impact2->unbind();

        LOG(INFO) << "Applying vertical blur:";
        go->glass_impact->bind();
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        go->blur_shader.begin();
        {
            texture_visualizer fbo_visualizer(float2{ 0.0f, 0.0f }, float2{ 1.f, 1.f });
            go->blur_shader.set_width_height(false, go->glass_impact2->get_width(), go->glass_impact2->get_height());
            fbo_visualizer.draw(go->blur_shader, textures[temp_glass_atlas].get());
        }

        go->blur_shader.end();
        go->glass_impact->unbind();

        go->rotated_tube = vao::create(apply(cylinder, r, { 0.f, 0.f, 0.f }, true));
        go->tube = vao::create(cylinder);
        go->cap = vao::create(cap);
        go->bent_tube = vao::create(bent_cylinder);
        go->grid = vao::create(make_grid(10, 10, 1.f, 1.f));
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

        with_texture(textures, cat_tex, 0, [&]() {
            go->cat->draw();
        });
    };

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

        with_texture(textures, world, 0, [&]() {
            go->earth->draw();
        });

        go->shader.set_model(mul(
            translation_matrix(float3{ 0.f, -1.f, 0.5f }),
            scaling_matrix(float3{ 1.f, 1.f, 1.f })
        ));
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        //go->white.bind(0);
        //go->grid->draw();
        //go->white.unbind();
        //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    };

    float last_t = 1.f;
    float4x4 tsp{
        { 1.f, 0.f, 0.f, 0.f },
        { 0.f, 1.f, 0.f, 0.f },
        { 0.f, 0.f, 1.f, 0.f },
        { 0.f, 0.f, 0.f, 1.f }
    };

    auto draw_glass_scatter = [&](int refraction_id, float t)
    {
        with_texture(textures, mish, go->tb_shader.diffuse_slot(), [&]() {
        with_texture(textures, normals, go->tb_shader.normal_map_slot(), [&]() {
        with_texture(textures, refraction_id, go->tb_shader.refraction_slot(), [&]() {


            go->tb_shader.enable_normal_mapping(false);
            go->tb_shader.set_decal_id(go->glass_id, glass_variations);

            //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            for (auto& g : go->glasses[go->glass_id])
            {
                auto st = t * 5.f;
                auto base = floor(st / 4.f) * 4.f;

                auto t0 = st - base;

                if (t0 < last_t)
                {
                    auto n = bent_cylinder.positions.size();

                    go->glass_id = rand() % go->glasses.size();

                    float3 to_camera;
                    float3 normal;
                    do
                    {
                        float4x4 mat = mul(
                            translation_matrix(float3{ 0.f, 0.f, -3.f }),
                            scaling_matrix(float3{ 1.f, 1.f, 1.f })
                        );

                        auto k = int(0.2 * n + rand() % int(n * 0.6));

                        auto pos = mul(mat, float4(bent_cylinder.positions[k], 1.0)).xyz();
                        auto cam_pos = cam->get_position();
                        to_camera = cam_pos - pos;

                        normal = normalize(bent_cylinder.normals[k]);
                        auto tangent = normalize(bent_cylinder.tangents[k]);
                        auto third = normalize(cross(normal, tangent));
                        tsp = {
                            { tangent, 0.f },
                            { third, 0.f },
                            { normal, 0.f },
                            { pos, 1.f }
                        };

                        auto uvs = bent_cylinder.uvs[k];

                        go->tb_shader.set_decal_uvs(uvs);

                    } while (dot(to_camera, normal) < 0.f);
                }
                last_t = t0;

                //t0 = t0 * (t0 - 0.1f);

                auto local_t = t0 * g.second.dist * 1.f;
                double w = cos(local_t * 7.0);
                float4 q(sinf(local_t * 7.0) * g.second.rotation, w);
                q = normalize(q);

                go->tb_shader.set_material_properties(diffuse_level * std::min(1.f + local_t, 1.5f),
                    shineDamper * std::min(1.f + local_t * 50.f, 60.f),
                    reflectivity * std::min(1.f + local_t * 50.f, 60.f));

                auto out_vec = 10.f * (g.second.pos - float3{ 0.5f, -0.5f, 0.f });

                auto view = mul(
                    scaling_matrix(float3{ 1.f, 1.f, 1.f }),
                    translation_matrix(float3{
                    -0.5f + g.second.pos.x + out_vec.x * local_t,
                    0.5f + g.second.pos.y + out_vec.y * local_t,
                    5.f * local_t - 0.03f }),
                    rotation_matrix(q)
                    );

                view = mul(tsp, view);

                go->tb_shader.set_model(view);

                g.first->draw();
            }
            go->tb_shader.enable_normal_mapping(true);
            go->tb_shader.set_material_properties(diffuse_level, shineDamper, reflectivity);

        });
        });
        });
    };

    auto draw_tubes = [&](int refraction_id, float t) {
        with_texture(textures, mish, go->tb_shader.diffuse_slot(), [&]() {
        with_texture(textures, normals, go->tb_shader.normal_map_slot(), [&]() {
        with_texture(textures, refraction_id, go->tb_shader.refraction_slot(), [&]() {

            go->tb_shader.enable_normal_mapping(true);

            go->tb_shader.set_model(mul(
                translation_matrix(float3{ 0.f, 0.f, 0.f }),
                scaling_matrix(float3{ 1.f, 1.f, 1.f })
            ));
            go->tube->draw();

            go->tb_shader.set_model(mul(
                translation_matrix(float3{ 0.f, 0.f, 5.f }),
                scaling_matrix(float3{ 1.f, 1.f, 1.f })
            ));
            //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            go->cap->draw();
            //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            go->tb_shader.set_model(mul(
                translation_matrix(float3{ 0.f, 0.f, 3.f }),
                scaling_matrix(float3{ 1.f, 1.f, 1.f })
            ));
            go->tube->draw();

            go->tb_shader.set_model(mul(
                translation_matrix(float3{ 0.f, 0.f, -3.f }),
                scaling_matrix(float3{ 1.f, 1.f, 1.f })
            ));

            with_texture(textures, final_glass_atlas, go->tb_shader.decal_atlas_slot(), [&]() {
                go->bent_tube->draw();
            });

            go->tb_shader.set_model(mul(
                translation_matrix(float3{ 5.f, 0.f, -0.f }),
                scaling_matrix(float3{ 1.f, 1.f, 1.f })
            ));
            tube_vaos[tube_idx * 2 + (fov <= 120 ? 1 : 0)]->draw();

        });
        });
        });
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

    while (app->is_alive() && !exit)
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        if (app->get_mouse().mouse_wheel && app->get_mouse().x >= 250)
        {
            fov = std::min(500.f, std::max(50.f, fov + app->get_mouse().mouse_wheel * 10));

            auto pos = cam->get_position();
            auto target = cam->get_target();

            cam = std::make_unique<camera>(*app, false, fov);

            cam->set_position(pos);
            cam->look_at(target);
            cam->update(*app, true);
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

        go->fbo1->bind();
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        draw_refractables(t);

        go->fbo1->unbind();

        go->fbo2->bind();
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        go->viz.draw_texture({ 0.f, 0.f }, { 1.f, 1.f }, textures[first_pass_color].get());

        glCullFace(GL_FRONT);

        go->shader.end();
        go->tb_shader.begin();

        go->tb_shader.set_material_properties(diffuse_level, shineDamper, reflectivity);
        go->tb_shader.set_light(l.position);
        go->tb_shader.set_mvp(matrix, cam->view_matrix(), cam->projection_matrix());
        go->tb_shader.set_distortion(0.2f);

        draw_tubes(first_pass_color, t);

        glCullFace(GL_BACK);

        go->tb_shader.end();
        go->shader.begin();

        draw_stuff_inside();

        go->shader.end();
        go->tb_shader.begin();

        go->fbo2->unbind();

        app->reset_viewport();
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        draw_tubes(first_pass_color, t);

        //--
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        go->tb_shader.set_distortion(0.1f);
        
        draw_tubes(second_pass_color, t);

        go->tb_shader.set_distortion(0.f);
        draw_glass_scatter(second_pass_color, t);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        go->tb_shader.end();
        go->shader.begin();

        draw_refractables(t);

        go->shader.end();

        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        const auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse;
        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ 250, static_cast<float>(app->height()) });

        ImGui::Begin("Control Panel", nullptr, flags);

        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

        ImGui::PushItemWidth(-1);
        if (ImGui::Button("Exit to Desktop", { 235, 0 }))
            exit = true;


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
            for (auto& to : textures)
            {
                total_bytes += to.get().get_bytes();
                load_time += to.get().get_load_time();
                texture_names.push_back(to.get_name().c_str());
            }
            auto bytes_str = bytes_to_string(total_bytes);
            ImGui::Text("Total Texture Memory: %s", bytes_str.c_str());
            ImGui::Text("Load Time: %f ms", load_time);

            auto& to = textures[selected_texture];
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

        for (auto& to : textures)
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
            for (auto& to : textures) to.release();
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