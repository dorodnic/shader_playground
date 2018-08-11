#include <imgui.h>
#include <imgui_impl_glfw.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include "window.h"

#include <easylogging++.h>

void GLAPIENTRY opengl_error_callback(GLenum source, GLenum type, GLuint id,
    GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    if (type == GL_DEBUG_TYPE_ERROR)
        LOG(ERROR) << message;
    else
        LOG(INFO) << message;
}

window::~window()
{
    ImGui::GetIO().Fonts->ClearFonts();  // To be refactored into Viewer theme object
    ImGui_ImplGlfw_Shutdown();

    glfwDestroyWindow(_window);
    glfwTerminate();
}

window::operator bool() 
{
    if (!_first)
    {
        ImGui::Render();
        glfwSwapBuffers(_window);
    }

    _first = false;

    if (glGetError() != GL_NO_ERROR)
        __debugbreak();

    auto res = !glfwWindowShouldClose(_window);

    _mouse.mouse_wheel = 0;
    glfwPollEvents();

    glfwGetFramebufferSize(_window, &_w, &_h);
    glViewport(0, 0, _w, _h);
    glfwGetWindowSize(_window, &_w, &_h);

    ImGui_ImplGlfw_NewFrame();

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

    ImGui_ImplGlfw_Init(_window, true);

    // Register for UI-controller events
    glfwSetWindowUserPointer(_window, this);


    glfwSetCursorPosCallback(_window, [](GLFWwindow* w, double cx, double cy)
    {
        auto data = reinterpret_cast<window*>(glfwGetWindowUserPointer(w));
        data->_mouse.x = (float)cx / data->_scale_factor;
        data->_mouse.y = (float)cy / data->_scale_factor;
    });
    glfwSetMouseButtonCallback(_window, [](GLFWwindow* w, int button, int action, int mods)
    {
        auto data = reinterpret_cast<window*>(glfwGetWindowUserPointer(w));
        data->_mouse.mouse_down = (button == GLFW_MOUSE_BUTTON_1) && (action != GLFW_RELEASE);
    });
    glfwSetScrollCallback(_window, [](GLFWwindow * w, double xoffset, double yoffset)
    {
        auto data = reinterpret_cast<window*>(glfwGetWindowUserPointer(w));
        data->_mouse.mouse_wheel = static_cast<int>(yoffset);
    });

    glfwSetDropCallback(_window, [](GLFWwindow* w, int count, const char** paths)
    {
        auto data = reinterpret_cast<window*>(glfwGetWindowUserPointer(w));

        if (count <= 0) return;

        for (int i = 0; i < count; i++)
        {
            data->on_file_drop(paths[i]);
        }
    });

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(opengl_error_callback, 0);
}