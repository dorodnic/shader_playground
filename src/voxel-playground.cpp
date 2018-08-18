#include <iostream>
#include <chrono>

#include "shader.h"
#include "window.h"
#include "texture.h"
#include "vao.h"
#include "camera.h"
#include "model.h"
#include "loader.h"

#include <easylogging++.h>
INITIALIZE_EASYLOGGINGPP

#include <imgui.h>

struct light
{
    float3 position;
    float3 colour;
};

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

    auto shader = shader_program::load("resources/shaders/vertex.glsl", 
                                       "resources/shaders/fragment.glsl");
    shader->bind_attribute(0, "position");
    shader->bind_attribute(1, "textureCoords");
    shader->bind_attribute(2, "normal");
    shader->bind_attribute(3, "tangent");

    auto transformation_matrix_location = shader->get_uniform_location("transformationMatrix");
    auto projection_matrix_location = shader->get_uniform_location("projectionMatrix");
    auto camera_matrix_location = shader->get_uniform_location("cameraMatrix");

    auto light_position_location = shader->get_uniform_location("lightPosition");
    auto light_colour_location = shader->get_uniform_location("lightColour");

    auto diffuse_location = shader->get_uniform_location("diffuse");
    auto shine_location = shader->get_uniform_location("shineDamper");
    auto reflectivity_location = shader->get_uniform_location("reflectivity");

    auto texture0_sampler_location = shader->get_uniform_location("textureSampler");
    auto texture1_sampler_location = shader->get_uniform_location("textureDarkSampler");
    auto texture_normal_sampler_location = shader->get_uniform_location("textureNormalSampler");
    auto texture_mask_sampler_location = shader->get_uniform_location("textureMaskSampler");

    shader->begin();
    shader->load_uniform(texture0_sampler_location, 0);
    shader->load_uniform(texture1_sampler_location, 1);
    shader->load_uniform(texture_normal_sampler_location, 2);
    shader->load_uniform(texture_mask_sampler_location, 3);
    shader->end();

    auto shineDamper = 10.f;
    auto reflectivity = 1.f;
    auto diffuse_level = 0.5f;
    auto light_angle = 0.f;
    bool rotate_light = true;

    camera cam(app);
    cam.look_at({ 0.f, 0.f, 0.f });

    loader earth("resources/earth.obj");
    bool loading = true;

    float3 arrow_vert[] = {
        { -0.05f, 0.f, 0.f },
        { 0.05f, 0.f, 0.f },
        { -0.05f, 0.7f, 0.f },
        { 0.05f, 0.7f, 0.f },
        { -0.2f, 0.7f, 0.f },
        { 0.2f, 0.7f, 0.f },
        { 0.0f, 1.f, 0.f },
    };
    int3 arrow_idx[] = {
        { 0, 1, 2 },
        { 1, 2, 3 },
        { 4, 5, 6 }
    };
    vao arrow(arrow_vert, nullptr, nullptr, nullptr,
        sizeof(arrow_vert) / sizeof(arrow_vert[0]),
        arrow_idx, sizeof(arrow_idx) / sizeof(arrow_idx[0]));

    auto arrow_shader = shader_program::load(
        "resources/shaders/arrow_vertex.glsl",
        "resources/shaders/arrow_fragment.glsl");
    arrow_shader->bind_attribute(0, "position");
    auto arrow_colour_location = arrow_shader->get_uniform_location("arrowColour");
    auto arrow_transformation_matrix_location = arrow_shader->get_uniform_location("transformationMatrix");
    auto arrow_projection_matrix_location = arrow_shader->get_uniform_location("projectionMatrix");
    auto arrow_camera_matrix_location = arrow_shader->get_uniform_location("cameraMatrix");

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

        shader->begin();

        shader->load_uniform(shine_location, shineDamper);
        shader->load_uniform(reflectivity_location, reflectivity);
        shader->load_uniform(diffuse_location, diffuse_level);

        shader->load_uniform(light_position_location, l.position);
        shader->load_uniform(light_colour_location, l.colour);

        shader->load_uniform(transformation_matrix_location, matrix);
        shader->load_uniform(camera_matrix_location, cam.view_matrix());
        shader->load_uniform(projection_matrix_location, cam.projection_matrix());

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

        shader->end();

        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        arrow_shader->begin();
        arrow_shader->load_uniform(arrow_camera_matrix_location, cam.view_matrix());
        arrow_shader->load_uniform(arrow_projection_matrix_location, cam.projection_matrix());
        
        /*if (earth.ready())
        {
            auto mesh = earth.get().front();

            for (int i = 0; i < mesh.positions.size(); i++)
            {
                auto pos = mesh.positions[i];
                auto norm = mesh.normals[i];
                auto matrix = mul(
                    translation_matrix(pos),
                    scaling_matrix(float3{ 0.2f, 0.2f, 0.2f })
                );
                norm = 0.2f * normalize(norm);

                auto vm = inverse(cam.view_matrix());
                auto to_camera = 0.2f * normalize(vm[3].xyz() - pos);

                if (dot(to_camera, mesh.normals[i]) < 0.f) continue;

                matrix[0].xyz() = cross(norm, to_camera);
                matrix[1].xyz() = norm;
                matrix[2].xyz() = to_camera;

                arrow_shader->load_uniform(arrow_transformation_matrix_location, matrix);
                arrow_shader->load_uniform(arrow_colour_location, { 1.f, 0.f, 0.f });

                arrow.draw();
            }

            for (int i = 0; i < mesh.positions.size(); i++)
            {
                auto pos = mesh.positions[i];
                auto norm = mesh.tangents[i];
                auto matrix = mul(
                    translation_matrix(pos),
                    scaling_matrix(float3{ 0.2f, 0.2f, 0.2f })
                );
                norm = 0.2f * normalize(norm);

                auto vm = inverse(cam.view_matrix());
                auto to_camera = 0.2f * normalize(vm[3].xyz() - pos);

                if (dot(to_camera, mesh.normals[i]) < 0.f) continue;

                matrix[0].xyz() = cross(norm, to_camera);
                matrix[1].xyz() = norm;
                matrix[2].xyz() = to_camera;

                arrow_shader->load_uniform(arrow_transformation_matrix_location, matrix);
                arrow_shader->load_uniform(arrow_colour_location, { 0.f, 0.f, 1.f });

                arrow.draw();
            }

            for (int i = 0; i < mesh.positions.size(); i++)
            {
                auto pos = mesh.positions[i];
                auto norm = cross(mesh.tangents[i], mesh.normals[i]);
                auto matrix = mul(
                    translation_matrix(pos),
                    scaling_matrix(float3{ 0.2f, 0.2f, 0.2f })
                );
                norm = 0.2f * normalize(norm);

                auto vm = inverse(cam.view_matrix());
                auto to_camera = 0.2f * normalize(vm[3].xyz() - pos);

                if (dot(to_camera, mesh.normals[i]) < 0.f) continue;

                matrix[0].xyz() = cross(norm, to_camera);
                matrix[1].xyz() = norm;
                matrix[2].xyz() = to_camera;

                arrow_shader->load_uniform(arrow_transformation_matrix_location, matrix);
                arrow_shader->load_uniform(arrow_colour_location, { 0.f, 1.f, 0.f });

                arrow.draw();
            }
        }*/

        arrow_shader->end();

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