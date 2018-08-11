#pragma once
#include "window.h"
#include "util.h"

#include <chrono>

class camera
{
public:
    camera(const window& w, float fov = 120.f, float n = 0.1f, float f = 10000.f);

    void update(const window& w);

    const float4x4& projection_matrix() const { return _projection; }
    const float4x4& view_matrix() const { return _view; }

    void set_position(float3 pos);
    void look_at(float3 at);

    float clock() const;

private:
    float3 _up{ 0.f, 1.f, 0.f };
    float3 _target{ 0.f, 0.f, 0.f };
    float3 _pos{ 0.f, 0.f, 3.f };
    float4x4 _view;
    mouse_info _prev_mouse;

    std::chrono::high_resolution_clock::time_point _start;

    float4x4 _projection;
};