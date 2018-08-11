#include <iostream>
#include <chrono>

#include "shader.h"
#include "window.h"
#include "texture.h"
#include "vao.h"
#include "camera.h"

#include <easylogging++.h>
INITIALIZE_EASYLOGGINGPP

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

    camera cam(app);

    while (app)
    {
        auto s = std::abs(std::sinf(cam.clock())) + 0.5f;

        auto matrix = mul(
            translation_matrix(float3{ 0.f, 0.f, 0.f }),
            scaling_matrix(float3{ s, s, s })
        );

        cam.update(app);

        shader->begin();
        shader->load_uniform(transformation_matrix_location, matrix);
        shader->load_uniform(camera_matrix_location, cam.view_matrix());
        shader->load_uniform(projection_matrix_location, cam.projection_matrix());
        obj.draw(mish);
        shader->end();
    }

    return EXIT_SUCCESS;
}