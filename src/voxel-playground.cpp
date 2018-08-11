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

int main(int argc, char* argv[])
{
    START_EASYLOGGINGPP(argc, argv);

    window app(1280, 720, "Voxel Playground");

    std::vector<vao> vaos;
    float max = 0.f;

    objl::Loader loader;
    bool loadout = loader.LoadFile("resources/house.obj");
    for (int i = 0; i < loader.LoadedMeshes.size(); i++)
    {
        objl::Mesh curMesh = loader.LoadedMeshes[i];
        LOG(INFO) << "Loaded mesh " << curMesh.MeshName;

        std::vector<float3> positions;
        for (auto&& v : curMesh.Vertices)
        {
            positions.emplace_back(v.Position.X, v.Position.Y, v.Position.Z);
            max = std::max(v.Position.X, max);
            max = std::max(v.Position.Y, max);
            max = std::max(v.Position.Z, max);
        }

        std::vector<float2> uvs;
        for (auto&& v : curMesh.Vertices)
            uvs.emplace_back(v.TextureCoordinate.X, v.TextureCoordinate.Y);
        std::vector<int3> idx;
        for (int i = 0; i < curMesh.Indices.size(); i+=3)
            idx.emplace_back(curMesh.Indices[i], curMesh.Indices[i+1], curMesh.Indices[i+2]);

        vao v(positions.data(), uvs.data(), positions.size(), idx.data(), idx.size());
        vaos.push_back(std::move(v));
    }

    /*float3 vertex[] = {
        { -0.5, 0.5, 0.f },
        { -0.5, -0.5, 0.f },
        { 0.5, -0.5, 0.f },
        { 0.5, 0.5, 0.f },
    };
    float2 uvs[] = {
        { 0, 0 },
        { 0, 1 },
        { 1, 1 },
        { 1, 0 }
    };
    int3 index[] = {
        { 0, 1, 3 },
        { 3, 1, 2}
    };
    vao obj(vertex, uvs, sizeof(vertex) / sizeof(vertex[0]),
            index, sizeof(index) / sizeof(index[0]));*/

    texture mish;
    mish.upload("resources/mish.jpg");

    auto shader = shader_program::load("resources/shaders/vertex.glsl", 
                                       "resources/shaders/fragment.glsl");
    shader->bind_attribute(0, "position");
    shader->bind_attribute(1, "textureCoords");
    auto transformation_matrix_location = shader->get_uniform_location("transformationMatrix");
    auto projection_matrix_location = shader->get_uniform_location("projectionMatrix");
    auto camera_matrix_location = shader->get_uniform_location("cameraMatrix");

    camera cam(app);
    cam.set_position({ 0.f, 0.f, -max });
    cam.look_at({ 0.f, 0.f, 0.f });

    while (app)
    {
        glEnable(GL_DEPTH_TEST);

        auto s = std::abs(std::sinf(cam.clock())) * 0.2f + 0.9f;

        auto matrix = mul(
            translation_matrix(float3{ 0.f, 0.f, 0.f }),
            scaling_matrix(float3{ s, s, s })
        );

        cam.update(app);

        shader->begin();
        shader->load_uniform(transformation_matrix_location, matrix);
        shader->load_uniform(camera_matrix_location, cam.view_matrix());
        shader->load_uniform(projection_matrix_location, cam.projection_matrix());

        for (auto&& vao : vaos)
            vao.draw(mish);

        shader->end();

        glDisable(GL_DEPTH_TEST);

        const auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse;
        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ 200, static_cast<float>(app.height()) });

        ImGui::Begin("Control Panel", nullptr, flags);

        ImGui::Text("Text");

        ImGui::End();
    }

    return EXIT_SUCCESS;
}