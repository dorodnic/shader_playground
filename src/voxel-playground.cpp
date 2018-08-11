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

struct triangle
{
    int idx[3];
};

enum class vbo_type
{
    array_buffer,
    element_array_buffer,
};

class vbo
{
public:
    vbo(vbo_type type = vbo_type::array_buffer);
    ~vbo();

    void upload(int attribute, float3* xyz, int count);
    void upload(triangle* indx, int count);

    void draw_triangles();
    void draw_indexed_triangles();

    void bind();
    void unbind();

private:
    static int convert_type(vbo_type type);

    uint32_t _id;
    uint32_t _size = 0;
    vbo_type _type;
};

int vbo::convert_type(vbo_type type)
{
    switch (type) {
    case vbo_type::array_buffer: return GL_ARRAY_BUFFER;
    case vbo_type::element_array_buffer: return GL_ELEMENT_ARRAY_BUFFER;
    default: throw std::runtime_error("Not supported VBO type!");
    }
}

vbo::vbo(vbo_type type)
    : _type(type)
{
    glGenBuffers(1, &_id);
}

void vbo::bind()
{
    glBindBuffer(convert_type(_type), _id);
}

void vbo::unbind()
{
    glBindBuffer(convert_type(_type), 0);
}

void vbo::upload(int attribute, float3* xyz, int count)
{
    assert(_type == vbo_type::array_buffer);
    bind();
    glBufferData(convert_type(_type), count * sizeof(float3), xyz, GL_STATIC_DRAW);
    glVertexAttribPointer(attribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
    _size = count;
    unbind();
}

void vbo::upload(triangle* indx, int count)
{
    assert(_type == vbo_type::element_array_buffer);
    bind();
    glBufferData(convert_type(_type), count * sizeof(triangle), indx, GL_STATIC_DRAW);
    _size = count;
}

void vbo::draw_triangles()
{
    assert(_type == vbo_type::array_buffer);
    bind();
    glDrawArrays(GL_TRIANGLES, 0, _size);
    unbind();
}

void vbo::draw_indexed_triangles()
{
    assert(_type == vbo_type::element_array_buffer);
    glDrawElements(GL_TRIANGLES, _size * (sizeof(triangle) / sizeof(int)), GL_UNSIGNED_INT, 0);
}

vbo::~vbo()
{
    glDeleteBuffers(1, &_id);
}

class vao
{
public:
    vao(float3* vert, int vert_count,
        triangle* indx, int indx_count);
    ~vao();
    void bind();
    void unbind();
    void draw();

private:
    uint32_t _id;
    vbo _vertexes;
    vbo _indexes;
};

vao::vao(float3* vert, int vert_count,
         triangle* indx, int indx_count)
    : _vertexes(vbo_type::array_buffer),
      _indexes(vbo_type::element_array_buffer)
{
    glGenVertexArrays(1, &_id);
    bind();
    _indexes.upload(indx, indx_count);
    _vertexes.upload(0, vert, vert_count);
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

    glEnableVertexAttribArray(0);
    _indexes.draw_indexed_triangles();
    glDisableVertexAttribArray(0);

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

class window
{
public:
    window(int w, int h, const char* title);
    ~window();

    operator bool();

    int width() const { return _w; }
    int height() const { return _h; }

private:
    GLFWwindow* _window;
    int _w, _h;
    bool _first = true;
};

window::~window()
{
    glfwDestroyWindow(_window);
    glfwTerminate();
}

window::operator bool() 
{
    if (!_first)
        glfwSwapBuffers(_window);

    _first = false;

    if (glGetError() != GL_NO_ERROR)
        __debugbreak();

    auto res = !glfwWindowShouldClose(_window);

    glfwPollEvents();

    glfwGetFramebufferSize(_window, &_w, &_h);
    glViewport(0, 0, _w, _h);
    glfwGetWindowSize(_window, &_w, &_h);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    return res;
}

window::window(int w, int h, const char* title)
    : _w(w), _h(h)
{
    if (!glfwInit()) {
        LOG(ERROR) << "Can't initialize GLFW!";
        throw std::runtime_error("Can't initialize GLFW!");
    }

    _window = glfwCreateWindow(_w, _h, title, nullptr, nullptr);
    glfwMakeContextCurrent(_window);

    glewExperimental = TRUE;
    if (glewInit() != GLEW_OK) {
        LOG(ERROR) << "Could not initialize GLEW!";
        throw std::runtime_error("Could not initialize GLEW!");
    }

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(opengl_error_callback, 0);
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
    triangle index[] = {
        { 0, 1, 3 },
        { 3, 1, 2}
    };
    vao obj(vertex, sizeof(vertex) / sizeof(vertex[0]),
            index, sizeof(index) / sizeof(index[0]));

    auto shader = shader_program::load("resources/shaders/vertex.glsl", 
                                       "resources/shaders/fragment.glsl");
    glBindAttribLocation(shader->get_id(), 0, "position");

    while (app)
    {
        

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


        shader->begin();
        obj.draw();
        shader->end();

        //ImGui::Render();

        
    }

    //ImGui_ImplGlfw_Shutdown();
    

    return EXIT_SUCCESS;
}