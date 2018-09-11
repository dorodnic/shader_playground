#include "glass-decals.h"

glass_decals_shader::glass_decals_shader()
    : texture_2d_shader(shader_program::load(
        "resources/shaders/planar/decal-atlas-vertex.glsl",
        "resources/shaders/planar/decal-atlas-fragment.glsl"))
{
    _width_location = _shader->get_uniform_location("imageWidth");
    _height_location = _shader->get_uniform_location("imageHeight");
    _horizontal_location = _shader->get_uniform_location("horizontal");
}

void glass_decals_shader::set_width_height(bool horizontal, int w, int h)
{
    _shader->load_uniform(_width_location, w);
    _shader->load_uniform(_height_location, h);
    _shader->load_uniform(_horizontal_location, horizontal ? 1.0f : 0.0f);
}

radial_shader::radial_shader()
    : texture_2d_shader(shader_program::load(
        "resources/shaders/planar/plane-vertex.glsl",
        "resources/shaders/planar/radial-fragment.glsl"))
{
    _decals_count_location = _shader->get_uniform_location("decalsCount");
}

void radial_shader::set_decals_count(int count)
{
    _shader->load_uniform(_decals_count_location, count);
}

normal_mapper_shader::normal_mapper_shader()
    : simple_shader(shader_program::load(
        "resources/shaders/simple/simp-vertex.glsl",
        "resources/shaders/simple/normal-mapper.glsl"))
{
}