#pragma once
#include <GLFW/glfw3.h>

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