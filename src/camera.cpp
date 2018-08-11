#include "camera.h"


#define ARCBALL_CAMERA_IMPLEMENTATION
#include <arcball_camera.h>

#include <imgui.h>

void camera::set_position(float3 pos)
{
    _pos = std::move(pos);
}

void camera::look_at(float3 at)
{
    _target = std::move(at);
}

camera::camera(const window& w, float fov, float n, float f)
{
    _projection = create_projection_matrix(w.width(), w.height(), 120, 0.1f, 1000.f);
    _view = identity_matrix();

    using namespace std::chrono;
    _start = high_resolution_clock::now();

    _prev_mouse = w.get_mouse();
}

float camera::clock() const
{
    using namespace std::chrono;
    auto now = high_resolution_clock::now();
    auto diff = duration_cast<milliseconds>(now - _start).count();
    auto elapsed_sec = diff / 1000.f;
    return elapsed_sec;
}

void camera::update(const window& w)
{
    auto dir = _target - _pos;
    auto size = length(dir);
    auto x_axis = cross(dir, _up);
    auto step = size * clock() / w.width();
    auto forward_step = dir * step;
    auto horizontal_step = x_axis * step;
    _up = { 0.f, 1.f, 0.f };

    if (ImGui::IsKeyPressed('w') || ImGui::IsKeyPressed('W'))
    {
        _pos += forward_step;
        _target += forward_step;
    }
    if (ImGui::IsKeyPressed('s') || ImGui::IsKeyPressed('S'))
    {
        _pos -= forward_step;
        _target -= forward_step;
    }
    if (ImGui::IsKeyPressed('d') || ImGui::IsKeyPressed('D'))
    {
        _pos += horizontal_step;
        _target += horizontal_step;
    }
    if (ImGui::IsKeyPressed('a') || ImGui::IsKeyPressed('A'))
    {
        _pos -= horizontal_step;
        _target -= horizontal_step;
    }

    arcball_camera_update(
        (float*)&_pos, (float*)&_target, (float*)&_up, (float*)&_view,
        clock(),
        0.2f, // zoom per tick
        -size / w.width(), // pan speed
        3.0f, // rotation multiplier
        w.width(), w.height(), // screen (window) size
        static_cast<int>(_prev_mouse.x), static_cast<int>(w.get_mouse().x),
        static_cast<int>(_prev_mouse.y), static_cast<int>(w.get_mouse().y),
        (ImGui::GetIO().MouseDown[2] || ImGui::GetIO().MouseDown[1]) ? 1 : 0,
        ImGui::GetIO().MouseDown[0] ? 1 : 0,
        w.get_mouse().mouse_wheel * size * 0.2f,
        0);

    _prev_mouse = w.get_mouse();
}