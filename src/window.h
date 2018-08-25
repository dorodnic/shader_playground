#pragma once
#include <GLFW/glfw3.h>

#include <functional>

struct mouse_info
{
    float x, y;
    bool mouse_down = false;
    int mouse_wheel = 0;
};

class window
{
public:
    window(int w, int h, const char* title, 
        int multisample = 4, bool fullscreen = false);
    ~window();

    bool is_alive();

    int width() const { return _w; }
    int height() const { return _h; }

    bool fullscreen() const { return _fullscreen; }
    int multisample() const { return _multisample; }

    void reset_viewport();

    const mouse_info& get_mouse() const { return _mouse; }
    std::function<void(std::string)> on_file_drop = [](std::string) {};
private:
    GLFWwindow* _window;
    int _w, _h;
    bool _first = true;
    mouse_info _mouse;
    float _scale_factor = 1.f;
    const int _multisample = 4;
    const bool _fullscreen = false;
};