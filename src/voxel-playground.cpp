#include <iostream>
#include <chrono>

#include "shader.h"
#include "window.h"
#include "texture.h"
#include "vao.h"

#include <easylogging++.h>
INITIALIZE_EASYLOGGINGPP

#define ARCBALL_CAMERA_IMPLEMENTATION
#include <arcball_camera.h>

#include <imgui.h>

#define PI 3.14159265

float to_rad(float deg)
{
    return deg * PI / 180;
}

float4x4 create_projection_matrix(float width, float height, float fov, float n, float f)
{
    auto ar = width / height;
    auto y_scale = (1.f / std::tanf(to_rad(fov / 2.f))) * ar;
    auto x_scale = y_scale / ar;
    auto length = f - n;

    float4x4 res;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            res[0][0] = 0.f;
    res[0][0] = x_scale;
    res[1][1] = y_scale;
    res[2][2] = -((f + n) / length);
    res[2][3] = -1;
    res[3][2] = -((2 * n * f) / length);
    return res;
}

int main(int argc, char* argv[])
{
    START_EASYLOGGINGPP(argc, argv);

    window app(1280, 720, "Voxel Playground");

    float3 vertex[] = {
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
            index, sizeof(index) / sizeof(index[0]));

    texture mish;
    mish.upload("resources/mish.jpg");

    auto shader = shader_program::load("resources/shaders/vertex.glsl", 
                                       "resources/shaders/fragment.glsl");
    shader->bind_attribute(0, "position");
    shader->bind_attribute(1, "textureCoords");
    auto transformation_matrix_location = shader->get_uniform_location("transformationMatrix");
    auto projection_matrix_location = shader->get_uniform_location("projectionMatrix");
    auto camera_matrix_location = shader->get_uniform_location("cameraMatrix");

    using namespace std::chrono;
    auto start = high_resolution_clock::now();

    float3 up{ 0.f, 1.f, 0.f };
    float3 target{ 0.f, 0.f, 0.f };
    float3 pos{ 0.f, 0.f, 3.f };
    float4x4 view;
    auto prev_mouse = app.get_mouse();

    auto projection = create_projection_matrix(app.width(), app.height(), 120, 0.1f, 1000.f);

    while (app)
    {
        auto now = high_resolution_clock::now();
        auto diff = duration_cast<milliseconds>(now - start).count();
        auto elapsed_sec = diff / 1000.f;

        auto s = std::abs(std::sinf(elapsed_sec));

        auto matrix = mul(
            translation_matrix(float3{ 0.f, 0.f, 0.f }),
            scaling_matrix(float3{ 1, 1, 1 })
        );

        arcball_camera_update(
            (float*)&pos, (float*)&target, (float*)&up, (float*)&view,
            elapsed_sec,
            0.2f, // zoom per tick
            -1.f / app.width(), // pan speed
            3.0f, // rotation multiplier
            app.width(), app.height(), // screen (window) size
            static_cast<int>(prev_mouse.x), static_cast<int>(app.get_mouse().x),
            static_cast<int>(prev_mouse.y), static_cast<int>(app.get_mouse().y),
            (ImGui::GetIO().MouseDown[2] || ImGui::GetIO().MouseDown[1]) ? 1 : 0,
            ImGui::GetIO().MouseDown[0] ? 1 : 0,
            app.get_mouse().mouse_wheel,
            0);

        shader->begin();
        shader->load_uniform(transformation_matrix_location, matrix);
        shader->load_uniform(camera_matrix_location, view);
        shader->load_uniform(projection_matrix_location, projection);
        obj.draw(mish);
        shader->end();

        prev_mouse = app.get_mouse();
    }

    return EXIT_SUCCESS;
}