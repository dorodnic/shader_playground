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
#include "fbo.h"

#include <easylogging++.h>
INITIALIZE_EASYLOGGINGPP

#include <imgui.h>

#include <math.h>

obj_mesh generate_tube(float length, float radius, int a, int b)
{
    obj_mesh res;

    float pi = 2 * acos(-1);
    int skipped = 0;

    auto toidx = [&](int i, int j) {
        return i * (b + 1) + j - skipped;
    };

    for (int i = 0; i <= a; i++)
    {
        auto ti = (float)i / a;
        for (int j = 0; j <= b; j++)
        {
            auto tj = (float)j / b;

            auto z = (length / 2) * ti + (-length / 2) * (1 - ti);

            auto x = sinf(tj * pi) * radius;
            auto y = -cosf(tj * pi) * radius;
            res.positions.emplace_back(x, y, z);

            auto l = sqrt(x * x + y * y);
            res.normals.emplace_back(x / l, y / l, 0);

            res.uvs.emplace_back(ti, tj);

            if (i < a && j < b)
            {
                auto curr = toidx(i, j);
                auto next_a = toidx(i + 1, j);
                auto next_b = toidx(i, j + 1);
                auto next_ab = toidx(i + 1, j + 1);
                res.indexes.emplace_back(curr, next_b, next_a);
                res.indexes.emplace_back(next_a, next_b, next_ab);
            }
        }
    }

    res.calculate_tangents();

    return res;
}

int main(int argc, char* argv[])
{
    START_EASYLOGGINGPP(argc, argv);

    window app(1280, 720, "Shader Playground");

    float progress = 0.f;

    texture mish;
    mish.upload("resources/texture.png");

    texture world;
    world.upload("resources/Diffuse_2K.png");

    auto cylinder = generate_tube(3.f, 1.f, 5, 15);
    auto cylinder_vao = vao::create(cylinder);

    loader ld("resources/earth.obj");
    while (!ld.ready());
    auto earth_vao = vao::create(ld.get().front());

    texture cat_tex;
    cat_tex.upload("resources/cat_diff.tga");
    loader cat_ld("resources/cat.obj");
    while (!cat_ld.ready());
    auto cat = vao::create(cat_ld.get().front());

    light l;
    l.position = { 100.f, 0.f, -20.f };
    l.colour = { 1.f, 0.f, 0.f };

    simple_shader shader;

    auto shineDamper = 10.f;
    auto reflectivity = 1.f;
    auto diffuse_level = 0.5f;
    auto light_angle = 0.f;
    bool rotate_light = true;

    camera cam(app);
    cam.look_at({ 0.f, 0.f, 0.f });

    fbo fbo1(640, 480);
    fbo1.createTextureAttachment();
    fbo1.createDepthTextureAttachment();
    fbo fbo2(640, 480);
    fbo2.createTextureAttachment();
    fbo2.createDepthTextureAttachment();

    float4x4 matrix;

    auto draw_refractables = [&]() {
        shader.set_model(mul(
            translation_matrix(float3{ 0.f, -9.f, -5.f }),
            scaling_matrix(float3{ 2.f, 2.f, 2.f })
        ));

        world.bind(0);
        earth_vao->draw();
        world.unbind();

        shader.set_model(mul(
            translation_matrix(float3{ 5.f, 0.f, -2.f }),
            scaling_matrix(float3{ 2.f, 2.f, 2.f })
        ));

        cat_tex.bind(0);
        cat->draw();
        cat_tex.unbind();
    };

    auto draw_tubes = [&](texture& color) {
        mish.bind(0);
        color.bind(1);
        shader.set_model(mul(
            translation_matrix(float3{ 0.f, 0.f, 0.f }),
            scaling_matrix(float3{ 1.f, 1.f, 1.f })
        ));
        cylinder_vao->draw();

        shader.set_model(mul(
            translation_matrix(float3{ 3.f, 0.f, 0.f }),
            scaling_matrix(float3{ 1.f, 1.f, 1.f })
        ));
        cylinder_vao->draw();

        color.unbind();
        mish.unbind();
    };

    while (app)
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        auto s = std::abs(std::sinf(cam.clock())) * 0.2f + 0.8f;
        auto t = std::abs(std::sinf(cam.clock() + 5)) * 0.2f + 0.8f;

        if (rotate_light)
        {
            light_angle = cam.clock();
            while (light_angle > 2 * 3.14) light_angle -= 2 * 3.14;
        }

        l.position = { 5.f * std::sinf(-light_angle), 1.5f, 5.f * std::cosf(-light_angle) };
        l.colour = { s, t, s };

        cam.update(app);

        shader.begin();

        shader.set_material_properties(diffuse_level, shineDamper, reflectivity);
        shader.set_light(l.position);
        shader.set_mvp(matrix, cam.view_matrix(), cam.projection_matrix());

        fbo1.bind();
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        draw_refractables();

        fbo1.unbind();


        fbo2.bind();
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        draw_refractables();

        glCullFace(GL_FRONT);

        draw_tubes(fbo1.get_color_texture());

        fbo2.unbind();

        app.reset_viewport();
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        draw_tubes(fbo1.get_color_texture());

        //--
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        draw_tubes(fbo2.get_color_texture());

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        draw_refractables();

        shader.end();

        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        const auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse;
        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ 250, static_cast<float>(app.height()) });

        ImGui::Begin("Control Panel", nullptr, flags);

        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

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

        ImGui::End();
    }

    return EXIT_SUCCESS;
}