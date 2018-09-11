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
