#include <imgui.h>
#include <imgui_impl_glfw.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include "shader.h"

#include <easylogging++.h>
INITIALIZE_EASYLOGGINGPP

struct float3
{
    float x, y, z;
};

class vbo
{
public:
    vbo(int attribute = 0);
    ~vbo();
    void upload(float3* xyz, int count);
    void draw_triangles();

private:
    void bind();
    void unbind();

    uint32_t _id;
    uint32_t _attribute;
    uint32_t _size = 0;
};

vbo::vbo(int attribute) : _attribute(attribute)
{
    glGenBuffers(1, &_id);
}

void vbo::bind()
{
    glBindBuffer(GL_ARRAY_BUFFER, _id);
}

void vbo::unbind()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void vbo::upload(float3* xyz, int count)
{
    bind();
    glEnableVertexAttribArray(_attribute);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(float3), xyz, GL_STATIC_DRAW);
    glVertexAttribPointer(_attribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glDisableVertexAttribArray(_attribute);
    _size = count;
    unbind();
}

void vbo::draw_triangles()
{
    bind();
    glEnableVertexAttribArray(_attribute);
    glDrawArrays(GL_TRIANGLES, 0, _size);
    glDisableVertexAttribArray(_attribute);
    unbind();
}

vbo::~vbo()
{
    glDeleteBuffers(1, &_id);
}

class vao
{
public:
    vao(float3* xyz, int count);
    ~vao();
    void bind();
    void unbind();
    void draw();

private:
    uint32_t _id;
    vbo _vertexes;
};

vao::vao(float3* xyz, int count)
{
    glGenVertexArrays(1, &_id);
    bind();
    _vertexes.upload(xyz, count);
    unbind();
}

vao::~vao()
{
    glDeleteVertexArrays(1, &_id);
}

void vao::bind()
{
    glBindVertexArray(_id);
}

void vao::unbind()
{
    glBindVertexArray(0);
}

void vao::draw()
{
    bind();
    _vertexes.draw_triangles();
    unbind();
}

void GLAPIENTRY opengl_error_callback(GLenum source, GLenum type, GLuint id,
    GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    if (type == GL_DEBUG_TYPE_ERROR)
        LOG(ERROR) << message;
    else
        LOG(INFO) << message;
}

int main(int argc, char* argv[])
{
    START_EASYLOGGINGPP(argc, argv);

    if (!glfwInit()) {
        LOG(ERROR) << "Can't initialize GLFW!";
        return EXIT_FAILURE;
    }

    auto window = glfwCreateWindow(1280, 720, "Voxel Playground", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    //ImGui_ImplGlfw_Init(window, true);

    glewExperimental = TRUE;
    if (glewInit() != GLEW_OK) {
        LOG(ERROR) << "Could not initialize GLEW!";
        return EXIT_FAILURE;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(opengl_error_callback, 0);

    float3 triangles[] = {
        { -0.5, 0.5, 0.f },
        { -0.5, -0.5, 0.f },
        { 0.5, -0.5, 0.f },
        { 0.5, -0.5, 0.f },
        { 0.5, 0.5, 0.f },
        { -0.5, 0.5, 0.f }
    };
    vao obj(triangles, sizeof(triangles) / sizeof(triangles[0]));

    auto shader = shader_program::load("resources/shaders/vertex.glsl", 
                                       "resources/shaders/fragment.glsl");
    glBindAttribLocation(shader->get_id(), 0, "position");

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        //glViewport(0, 0, w, h);

        //glMatrixMode(GL_PROJECTION);
        //glLoadIdentity();

        //ImGui_ImplGlfw_NewFrame();
        //const auto flags = ImGuiWindowFlags_NoResize |
        //    ImGuiWindowFlags_NoMove |
        //    ImGuiWindowFlags_NoCollapse;
        //ImGui::SetNextWindowPos({ 0, 0 });
        //ImGui::SetNextWindowSize({ 200, static_cast<float>(h) });

        //ImGui::Begin("Control Panel", nullptr, flags);

        //

        //ImGui::End();

        // Rendering
        /*glViewport(0, 0,
            static_cast<int>(ImGui::GetIO().DisplaySize.x),
            static_cast<int>(ImGui::GetIO().DisplaySize.y));*/

        glfwGetWindowSize(window, &w, &h);


        //glClearColor(1, 0, 0, 1);
        //glClear(GL_COLOR_BUFFER_BIT);


        shader->begin();
        obj.draw();
        shader->end();

        //ImGui::Render();

        glfwSwapBuffers(window);

        if (glGetError() != GL_NO_ERROR)
            __debugbreak();
    }

    //ImGui_ImplGlfw_Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}