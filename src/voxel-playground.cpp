#include <imgui.h>
#include <imgui_impl_glfw.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include "shader.h"
#include "window.h"
#include "texture.h"
#include "vao.h"

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
    triangle index[] = {
        { 0, 1, 3 },
        { 3, 1, 2}
    };
    vao obj(vertex, uvs, sizeof(vertex) / sizeof(vertex[0]),
            index, sizeof(index) / sizeof(index[0]));

    texture mish;
    mish.upload("resources/mish.jpg");

    auto shader = shader_program::load("resources/shaders/vertex.glsl", 
                                       "resources/shaders/fragment.glsl");
    glBindAttribLocation(shader->get_id(), 0, "position");
    glBindAttribLocation(shader->get_id(), 1, "textureCoords");

    while (app)
    {
        shader->begin();
        obj.draw(mish);
        shader->end();
    }

    return EXIT_SUCCESS;
}