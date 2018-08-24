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

#include <easylogging++.h>
INITIALIZE_EASYLOGGINGPP

#include <imgui.h>

#include <math.h>

obj_mesh generate_tube(float length, float radius, int a, int b)
{
    obj_mesh res;

    float pi = 2 * acos(-1);

    auto toidx = [&](int i, int j) {
        return i * (b + 1) + j;
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

    window app(1280, 720, "Voxel Playground");

    float progress = 0.f;

    texture mish;
    mish.upload("resources/mish.jpg");

    auto cylinder = generate_tube(3.f, 1.f, 5, 15);
    auto cylinder_vao = vao::create(cylinder);

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

        auto matrix = mul(
            translation_matrix(float3{ 0.f, 0.f, 0.f }),
            scaling_matrix(float3{ 1.f, 1.f, 1.f })
        );

        cam.update(app);

        shader.begin();
        shader.set_material_properties(diffuse_level, shineDamper, reflectivity);
        shader.set_light(l.position);
        shader.set_mvp(matrix, cam.view_matrix(), cam.projection_matrix());

        mish.bind(0);
        cylinder_vao->draw();
        mish.unbind();

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