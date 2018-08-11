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