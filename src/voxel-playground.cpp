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

#include <easylogging++.h>
INITIALIZE_EASYLOGGINGPP

#include <imgui.h>

#include <math.h>

class plane_2d
{
public:
    plane_2d(float2 pos, float2 scale)
        : _position(std::move(pos)), 
          _scale(std::move(scale)),
          _geometry(vao::create(create_mesh()))
    {

    }

    void draw(texture_2d_shader& shader, texture& tex)
    {
        shader.begin();
        shader.set_position_and_scale(_position, _scale);
        tex.bind(0);
        _geometry->draw();
        tex.unbind();
        shader.end();
    }

private:
    static obj_mesh create_mesh()
    {
        obj_mesh res;

        res.positions.emplace_back(-1.f, -1.f, 0.f);
        res.positions.emplace_back(1.f, -1.f, 0.f);
        res.positions.emplace_back(1.f, 1.f, 0.f);
        res.positions.emplace_back(-1.f, 1.f, 0.f);
        res.normals.resize(4, { 0.f, 0.f, 0.f });
        res.tangents.resize(4, { 0.f, 0.f, 0.f });
        
        res.uvs.emplace_back(0.f, 0.f);
        res.uvs.emplace_back(1.f, 0.f);
        res.uvs.emplace_back(1.f, 1.f);
        res.uvs.emplace_back(-0.f, 1.f);

        res.indexes.emplace_back(0, 1, 2);
        res.indexes.emplace_back(2, 3, 0);

        return res;
    }

    float2 _position;
    float2 _scale;
    std::shared_ptr<vao> _geometry;
};

struct graphic_objects
{
    texture mish, normals, world, cat_tex;
    std::shared_ptr<vao> earth, tube, bent_tube, cat;
    simple_shader shader;
    tube_shader tb_shader;
    texture_2d_shader tex_2d_shader;
    std::shared_ptr<fbo> fbo1, fbo2;

    std::shared_ptr<plane_2d> fdo_visualizer, fdo2_visualizer;
};

int main(int argc, char* argv[])
{
    START_EASYLOGGINGPP(argc, argv);

    auto app = std::make_unique<window>(1280, 720, "Shader Playground");

    float progress = 0.f;

    auto go = std::make_shared<graphic_objects>();

    bool mipmap = true;
    bool linear = true;

    auto reload_textures = [&]() {
        go->mish.set_options(linear, mipmap);
        go->mish.upload("resources/texture.png");

        go->normals.set_options(linear, mipmap);
        go->normals.upload("resources/normal_map.png");

        go->world.set_options(linear, mipmap);
        go->world.upload("resources/Diffuse_2K.png");

        go->cat_tex.set_options(linear, mipmap);
        go->cat_tex.upload("resources/cat_diff.tga");

        go->fbo1->get_color_texture().set_options(linear, mipmap);
        go->fbo2->get_color_texture().set_options(linear, mipmap);
    };

    auto cylinder = generate_tube(3.f, 1.f, 0, 1, 16);
    auto bent_cylinder = generate_tube(3.f, 1.f, -1.f, 32, 16);

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
    float fov = 120.f;

    auto cam = std::make_unique<camera>(*app);
    cam->look_at({ 0.f, 0.f, 0.f });

    int2 fbo_resolutions[] = {
        { 120, 90 },
        { 320, 180 },
        { 640, 480 },
        { 800, 600 },
        { 1280, 720 },
    };
    const char* fbo_resolution_names[] = { "120 x 90", "320 x 180", "640 x 480", "800 x 600", "1280 x 720" };
    int fbo_resolution = 2;
    int total_fbo_res = sizeof(fbo_resolutions) / sizeof(fbo_resolutions[0]);

    auto reload_fbos = [&]() {
        go->fbo1 = std::make_shared<fbo>(
            fbo_resolutions[fbo_resolution].x, 
            fbo_resolutions[fbo_resolution].y);
        go->fbo1->createTextureAttachment();
        go->fbo1->createDepthTextureAttachment();

        go->fbo2 = std::make_shared<fbo>(
            fbo_resolutions[fbo_resolution].x,
            fbo_resolutions[fbo_resolution].y);
        go->fbo2->createTextureAttachment();
        go->fbo2->createDepthTextureAttachment();
    };

    auto reload_graphics = [&]() {
        go = std::make_shared<graphic_objects>();
        go->tube = vao::create(cylinder);
        go->bent_tube = vao::create(bent_cylinder);
        go->cat = vao::create(cat_ld.get().front());
        go->earth = vao::create(ld.get().front());

        reload_fbos();
        reload_textures();

        go->fdo_visualizer = std::make_shared<plane_2d>(
            float2{ 0.8f, 0.8f }, float2{ 0.2f, 0.2f });
        go->fdo2_visualizer = std::make_shared<plane_2d>(
            float2{ 0.8f, 0.4f }, float2{ 0.2f, 0.2f });
    };
    reload_graphics();

    float4x4 matrix;

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

        go->world.bind(0);
        go->earth->draw();
        go->world.unbind();

        go->shader.set_model(mul(
            translation_matrix(float3{ 0.f, -0.6f, -2.f }),
            scaling_matrix(float3{ 1.5f, 1.5f, 1.5f })
        ));

        go->cat_tex.bind(0);
        go->cat->draw();
        go->cat_tex.unbind();
    };

    auto draw_tubes = [&](texture& color) {
        go->mish.bind(0);
        color.bind(2);
        go->normals.bind(1);
        go->tb_shader.set_model(mul(
            translation_matrix(float3{ 0.f, 0.f, 0.f }),
            scaling_matrix(float3{ 1.f, 1.f, 1.f })
        ));
        go->tube->draw();

        go->tb_shader.set_model(mul(
            translation_matrix(float3{ 0.f, 0.f, 3.f }),
            scaling_matrix(float3{ 1.f, 1.f, 1.f })
        ));
        go->tube->draw();

        go->tb_shader.set_model(mul(
            translation_matrix(float3{ 0.f, 0.f, -3.f }),
            scaling_matrix(float3{ 1.f, 1.f, 1.f })
        ));
        go->bent_tube->draw();
        //go->tube->draw();

        go->tb_shader.set_model(mul(
            translation_matrix(float3{ -3.f, 0.f, -4.f }),
            scaling_matrix(float3{ 1.f, 1.f, 1.f })
        ));
        go->tube->draw();

        color.unbind();
        go->mish.unbind();
        go->normals.unbind();
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

    while (app->is_alive() && !exit)
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

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

        draw_refractables(t);

        glCullFace(GL_FRONT);

        go->shader.end();
        go->tb_shader.begin();

        go->tb_shader.set_material_properties(diffuse_level, shineDamper, reflectivity);
        go->tb_shader.set_light(l.position);
        go->tb_shader.set_mvp(matrix, cam->view_matrix(), cam->projection_matrix());
        go->tb_shader.set_distortion(0.2f);

        draw_tubes(go->fbo1->get_color_texture());

        go->fbo2->unbind();

        app->reset_viewport();
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        draw_tubes(go->fbo1->get_color_texture());

        //--
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        go->tb_shader.set_distortion(0.f);
        draw_tubes(go->fbo2->get_color_texture());

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        go->tb_shader.end();
        go->shader.begin();

        draw_refractables(t);

        go->shader.end();

        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        go->fdo_visualizer->draw(go->tex_2d_shader, go->fbo1->get_color_texture());
        go->fdo2_visualizer->draw(go->tex_2d_shader, go->fbo2->get_color_texture());

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

        ImGui::End();

        if (do_reset)
        {
            LOG(INFO) << "Releasing all OpenGL objects and Window";
            go.reset();
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