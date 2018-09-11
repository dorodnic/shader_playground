#pragma once

#include "texture-2d-shader.h"
#include "simple-shader.h"

class glass_decals_shader : public texture_2d_shader
{
public:
    glass_decals_shader();

    void set_width_height(bool horizontal, int w, int h);
private:
    uint32_t _width_location, _height_location, _horizontal_location;
};

class radial_shader : public texture_2d_shader
{
public:
    radial_shader();

    void set_decals_count(int count);
private:
    uint32_t _decals_count_location;
};

class normal_mapper_shader : public simple_shader
{
public:
    normal_mapper_shader();
};