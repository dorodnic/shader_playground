#include "texture-2d-shader.h"

texture_2d_shader::texture_2d_shader(std::unique_ptr<shader_program> shader)
    : _shader(std::move(shader))
{
    init();
}

texture_2d_shader::texture_2d_shader()
{
    _shader = shader_program::load(
        "resources/shaders/planar/plane-vertex.glsl",
        "resources/shaders/planar/plane-fragment.glsl");

    init();
}

void texture_2d_shader::set_position_and_scale(
    const float2& position,
    const float2& scale)
{
    _shader->load_uniform(_position_location, position);
    _shader->load_uniform(_scale_location, scale);
}

void texture_2d_shader::init()
{
    _shader->bind_attribute(0, "position");
    _shader->bind_attribute(1, "textureCoords");

    _position_location = _shader->get_uniform_location("elementPosition");
    _scale_location = _shader->get_uniform_location("elementScale");

    auto texture0_sampler_location = _shader->get_uniform_location("textureSampler");

    _shader->begin();
    _shader->load_uniform(texture0_sampler_location, 0);
    _shader->end();
}

void texture_2d_shader::begin() { _shader->begin(); }
void texture_2d_shader::end() { _shader->end(); }

gaussian_blur::gaussian_blur()
    : texture_2d_shader(shader_program::load(
        "resources/shaders/gaussian/blur-vertex.glsl",
        "resources/shaders/gaussian/blur-fragment.glsl"))
{
    _width_location = _shader->get_uniform_location("imageWidth");
    _height_location = _shader->get_uniform_location("imageHeight");
    _horizontal_location = _shader->get_uniform_location("horizontal");
}

void gaussian_blur::set_width_height(bool horizontal, int w, int h)
{
    _shader->load_uniform(_width_location, w);
    _shader->load_uniform(_height_location, h);
    _shader->load_uniform(_horizontal_location, horizontal ? 1.0f : 0.0f);
}
