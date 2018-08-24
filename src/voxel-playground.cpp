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

#include <easylogging++.h>
INITIALIZE_EASYLOGGINGPP

#include <imgui.h>

int main(int argc, char* argv[])
{
    START_EASYLOGGINGPP(argc, argv);

    window app(1280, 720, "Voxel Playground");

    std::vector<model> models;
    models.emplace_back("*");

    float progress = 0.f;

    texture diffuse;
    texture diffuse2;
    texture normal_map;
    texture ocean_mask;

    diffuse.upload("resources/Diffuse_2K.png");
    diffuse2.upload("resources/Night_lights_2K.png");
    normal_map.upload("resources/Normal_2K.png");
    ocean_mask.upload("resources/mish.jpg");

    light l;
    l.position = { 100.f, 0.f, -20.f };
    l.colour = { 1.f, 0.f, 0.f };

    advanced_shader shader;

    auto shineDamper = 10.f;
    auto reflectivity = 1.f;
    auto diffuse_level = 0.5f;
    auto light_angle = 0.f;
    bool rotate_light = true;

    camera cam(app);
    cam.look_at({ 0.f, 0.f, 0.f });

    loader earth("resources/earth.obj");
    bool loading = true;

    while (app)
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        if (earth.ready() && loading)
        {
            for (auto& model : models)
                model.create(earth.get());
            loading = false;
        }

        auto s = std::abs(std::sinf(cam.clock())) * 0.2f + 0.8f;
        auto t = std::abs(std::sinf(cam.clock() + 5)) * 0.2f + 0.8f;

        if (rotate_light)
        {
            light_angle = cam.clock();
            while (light_angle > 2 * 3.14) light_angle -= 2 * 3.14;
        }

        l.position = { 2.f * std::sinf(-light_angle), 1.5f, 2.f * std::cosf(-light_angle) };
        l.colour = { s, t, s };

        auto matrix = mul(
            translation_matrix(float3{ 0.f, 0.f, 0.f }),
            scaling_matrix(float3{ 1.f, 1.f, 1.f })
        );

        cam.update(app);

        shader.begin();
        shader.set_material_properties(diffuse_level, shineDamper, reflectivity);
        shader.set_light(l);
        shader.set_mvp(matrix, cam.view_matrix(), cam.projection_matrix());

        diffuse.bind(0);
        diffuse2.bind(1);
        normal_map.bind(2);
        ocean_mask.bind(3);
        
        for (auto& model : models)
        {
            model.render();
        }

        ocean_mask.unbind();
        normal_map.unbind();
        diffuse2.unbind();
        diffuse.unbind();

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

        if (loading)
        {
            ImGui::Text("Loading Assets...");
            ImGui::PushItemWidth(-1);
            ImGui::ProgressBar(progress);
        }
        else
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

        for (auto& model : models)
        {
            std::stringstream ss; 
            ss << model.id << "##" << "_visible";
            ImGui::Checkbox(ss.str().c_str(), &model.visible);
        }

        ImGui::End();
    }

    return EXIT_SUCCESS;
}