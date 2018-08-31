#pragma once

#include "util.h"
#include "loader.h"

obj_mesh apply(const obj_mesh& input, const float3x3& trans, const float3& t = { 0.f, 0.f, 0.f }, bool flip_normals = false);
obj_mesh fuse(const obj_mesh& a, const obj_mesh& b);
obj_mesh filter(const obj_mesh& a, std::function<bool(const float3&)> pred, std::vector<int>& edge);
int nearest(const obj_mesh& a, const std::vector<int>& subset, int i);

obj_mesh make_grid(int a, int b, float x, float y);

obj_mesh generate_tube(float length,
    float radius, float bend, int a, int b);
obj_mesh generate_tube3(float length,
    float radius, int a, int b);
obj_mesh generate_tube4(float length,
    float radius, float bend, int a, int b);