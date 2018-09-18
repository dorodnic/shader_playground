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

camera::camera(const window& w, bool perspective, float fov, float n, float f)
    : _perspective(perspective)
{
    if (perspective)
    {
        _projection = create_perspective_projection_matrix(w.width(), w.height(), fov, 0.1f, 1000.f);
    }
    else
    {
        _projection = create_orthographic_projection_matrix(w.width(), w.height(), fov, 0.1f, 1000.f);
    }
    
    _view = identity_matrix();

    _prev_mouse = w.get_mouse();
}

void camera::update(const window& w, bool force)
{
    _up = { 0.f, 1.f, 0.f };
    auto dir = _target - _pos;
    auto size = length(dir);
    auto x_axis = cross(dir, _up);
    auto step = size * clock() / w.width();
    auto forward_step = dir * step;
    auto horizontal_step = x_axis * step;
    //

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

    if (w.get_mouse().x > 250 || force)
    {
        arcball_camera_update(
            (float*)&_pos, (float*)&_target, (float*)&_up, (float*)&_view,
            w.get_time(),
            0.2f, // zoom per tick
            -size / w.width(), // pan speed
            3.0f, // rotation multiplier
            w.width(), w.height(), // screen (window) size
            static_cast<int>(_prev_mouse.x), static_cast<int>(w.get_mouse().x),
            static_cast<int>(_prev_mouse.y), static_cast<int>(w.get_mouse().y),
            (ImGui::GetIO().MouseDown[2]) ? 1 : 0,
            ImGui::GetIO().MouseDown[0] ? 1 : 0,
            w.get_mouse().mouse_wheel,
            0);
    }

    _prev_mouse = w.get_mouse();
}