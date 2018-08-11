#include <iostream>
#include <chrono>

#include "shader.h"
#include "window.h"
#include "texture.h"
#include "vao.h"
#include "camera.h"

#include <easylogging++.h>
INITIALIZE_EASYLOGGINGPP

#include <imgui.h>

#include <OBJ_Loader.h>

#include <readerwriterqueue.h>

struct light
{
    float3 position;
    float3 colour;
};

int main(int argc, char* argv[])
{
    START_EASYLOGGINGPP(argc, argv);

    window app(1280, 720, "Voxel Playground");

    std::vector<vao> vaos;
    float max = 1.f;
    float progress = 0.f;
    int vertex_count = 0;

    texture diffuse;
    diffuse.upload("resources/Diffuse_2K.png");
    texture diffuse2;
    diffuse2.upload("resources/Night_lights_2K.png");

    moodycamel::ReaderWriterQueue<objl::Loader> queue;

    std::thread loader_thread([&]() {
        objl::Loader loader;
        bool loadout = loader.LoadFile("resources/earth.obj",
            [&](float p) { progress = p; });
        LOG(INFO) << "Done loading model";
        queue.enqueue(std::move(loader));
    });

    auto read_from_loader = [&](const objl::Loader& loader) {
        for (int i = 0; i < loader.LoadedMeshes.size(); i++)
        {
            objl::Mesh curMesh = loader.LoadedMeshes[i];
            LOG(INFO) << "Loaded mesh " << curMesh.MeshName;

            if (curMesh.MeshMaterial.map_Kd != "")
            {
                auto dir = get_directory(loader.Path);
                auto diffuse_path = curMesh.MeshMaterial.map_Kd;
                if (dir != "") diffuse_path = dir + "/" + diffuse_path;

                diffuse.upload(diffuse_path);
            }

            std::vector<float3> positions;
            for (auto&& v : curMesh.Vertices)
            {
                positions.emplace_back(v.Position.X, v.Position.Y, v.Position.Z);
                max = std::max(v.Position.X, max);
                max = std::max(v.Position.Y, max);
                max = std::max(v.Position.Z, max);
                vertex_count++;
            }

            std::vector<float3> normals;
            for (auto&& v : curMesh.Vertices)
                normals.emplace_back(v.Normal.X, v.Normal.Y, v.Normal.Z);
            std::vector<float2> uvs;
            for (auto&& v : curMesh.Vertices)
                uvs.emplace_back(v.TextureCoordinate.X, v.TextureCoordinate.Y);
            std::vector<int3> idx;
            for (int i = 0; i < curMesh.Indices.size(); i += 3)
                idx.emplace_back(curMesh.Indices[i], curMesh.Indices[i + 1], curMesh.Indices[i + 2]);

            vao v(positions.data(), uvs.data(), normals.data(),
                positions.size(), idx.data(), idx.size());
            vaos.push_back(std::move(v));
        }
    };

    light l;
    l.position = { 100.f, 0.f, -20.f };
    l.colour = { 1.f, 0.f, 0.f };

    auto shader = shader_program::load("resources/shaders/vertex.glsl", 
                                       "resources/shaders/fragment.glsl");
    shader->bind_attribute(0, "position");
    shader->bind_attribute(1, "textureCoords");
    shader->bind_attribute(2, "normal");

    auto transformation_matrix_location = shader->get_uniform_location("transformationMatrix");
    auto projection_matrix_location = shader->get_uniform_location("projectionMatrix");
    auto camera_matrix_location = shader->get_uniform_location("cameraMatrix");

    auto light_position_location = shader->get_uniform_location("lightPosition");
    auto light_colour_location = shader->get_uniform_location("lightColour");

    auto shine_location = shader->get_uniform_location("shineDamper");
    auto reflectivity_location = shader->get_uniform_location("reflectivity");

    auto shineDamper = 10.f;
    auto reflectivity = 1.f;

    camera cam(app);
    cam.look_at({ 0.f, 0.f, 0.f });

    bool loading = true;

    while (app)
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        objl::Loader loader;
        if (queue.try_dequeue(loader))
        {
            read_from_loader(loader);
            cam.set_position({ 0.f, 0.f, -1.5f * max });
            loading = false;
        }

        auto s = std::abs(std::sinf(cam.clock())) * 0.2f + 0.8f;
        auto t = std::abs(std::sinf(cam.clock() + 5)) * 0.2f + 0.8f;

        l.position = { 2.f * max * std::sinf(-cam.clock()), 0.f, 2.f * max * std::cosf(-cam.clock()) };
        l.colour = { s, t, s };

        auto matrix = mul(
            translation_matrix(float3{ 0.f, 0.f, 0.f }),
            scaling_matrix(float3{ 1.f, 1.f, 1.f })
        );

        cam.update(app);

        shader->begin();

        shader->load_uniform(shine_location, shineDamper);
        shader->load_uniform(reflectivity_location, reflectivity);

        shader->load_uniform(light_position_location, l.position);
        shader->load_uniform(light_colour_location, l.colour);

        shader->load_uniform(transformation_matrix_location, matrix);
        shader->load_uniform(camera_matrix_location, cam.view_matrix());
        shader->load_uniform(projection_matrix_location, cam.projection_matrix());

        diffuse.bind(0);
        diffuse2.bind(1);
        for (auto&& vao : vaos)
        {
            vao.draw();
        }
        diffuse2.unbind();
        diffuse.unbind();

        shader->end();

        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        const auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse;
        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ 250, static_cast<float>(app.height()) });

        ImGui::Begin("Control Panel", nullptr, flags);

        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Vertex Count: %d", vertex_count);

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
            ImGui::SliderFloat("##Reflect", &reflectivity, 0.f, 1.f);
        }

        ImGui::End();
    }

    loader_thread.join();

    return EXIT_SUCCESS;
}