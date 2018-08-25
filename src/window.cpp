#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>

#include "window.h"
#include "util.h"

#include <easylogging++.h>

void APIENTRY opengl_error_callback(GLenum source, GLenum type, GLuint id,
    GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    if (type == GL_DEBUG_TYPE_ERROR)
        LOG(ERROR) << message;
    else
        LOG(INFO) << message;
}

void window::reset_viewport()
{
    glfwGetFramebufferSize(_window, &_w, &_h);
    glViewport(0, 0, _w, _h);
    glfwGetWindowSize(_window, &_w, &_h);
    if (_multisample) glEnable(GL_MULTISAMPLE);
}

window::~window()
{
    if (!_first)
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(_window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(_window);
    glfwTerminate();
}

bool window::is_alive()
{
    if (!_first)
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(_window);
    }

    _first = false;

    if (glGetError() != GL_NO_ERROR)
        __debugbreak();

    auto res = !glfwWindowShouldClose(_window);

    _mouse.mouse_wheel = 0;
    glfwPollEvents();

    reset_viewport();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    return res;
}

double window::get_time() const
{
    return glfwGetTime();
}

window::window(int w, int h, const char* title, int multisample, bool fullscreen)
    : _w(w), _h(h), _fullscreen(fullscreen), _multisample(multisample)
{
    if (!glfwInit()) {
        throw util_exception("Can't initialize GLFW!");
    }

    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    glfwWindowHint(GLFW_SAMPLES, _multisample);

    _window = glfwCreateWindow(_w, _h, title, 
        fullscreen ? glfwGetPrimaryMonitor() : nullptr, 
        nullptr);
    glfwMakeContextCurrent(_window);
    glfwSwapInterval(0);

    if (gl3wInit()) {
        throw util_exception("Can't initialize OpenGL!");
    }

    if (!gl3wIsSupported(3, 2)) {
        throw util_exception("OpenGL 3.2 not supported!");
    }

    LOG(INFO) << "OpenGL " << glGetString(GL_VERSION) << ", GLSL " <<
        glGetString(GL_SHADING_LANGUAGE_VERSION);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Setup style
    ImGui::StyleColorsDark();

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