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
    window(int w, int h, const char* title);
    ~window();

    operator bool();

    int width() const { return _w; }
    int height() const { return _h; }

    const mouse_info& get_mouse() const { return _mouse; }
    std::function<void(std::string)> on_file_drop = [](std::string) {};
private:
    GLFWwindow* _window;
    int _w, _h;
    bool _first = true;
    mouse_info _mouse;
    float _scale_factor = 1.f;
};