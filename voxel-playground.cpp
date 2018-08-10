#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>

#pragma comment(lib, "opengl32.lib")

#include <iostream>

int main()
{
    if (!glfwInit()) std::cerr << "Can't initialize GLFW!" << std::endl;

    auto window = glfwCreateWindow(1280, 720, "Voxel Playground", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    ImGui_ImplGlfw_Init(window, true);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        int w, h;
        glfwGetWindowSize(window, &w, &h);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        ImGui_ImplGlfw_NewFrame();
        const auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse;
        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ 200, static_cast<float>(h) });

        ImGui::Begin("Control Panel", nullptr, flags);

        

        ImGui::End();

        // Rendering
        glViewport(0, 0,
            static_cast<int>(ImGui::GetIO().DisplaySize.x),
            static_cast<int>(ImGui::GetIO().DisplaySize.y));
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwGetWindowSize(window, &w, &h);
        glLoadIdentity();
        glOrtho(0, w, h, 0, -1, +1);

        ImGui::Render();

        glfwSwapBuffers(window);
    }

    ImGui_ImplGlfw_Shutdown();
    glfwTerminate();

    return EXIT_SUCCESS;
}