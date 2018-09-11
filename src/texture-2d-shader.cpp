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

void texture_visualizer::draw(texture_2d_shader& shader, texture& tex)
{
    shader.begin();
    shader.set_position_and_scale(_position, _scale);
    tex.bind(0);
    _geometry->draw();
    tex.unbind();
    shader.end();
}

obj_mesh texture_visualizer::create_mesh()
{
    obj_mesh res;

    res.positions.emplace_back(-1.f, -1.f, 0.f);
    res.positions.emplace_back(1.f, -1.f, 0.f);
    res.positions.emplace_back(1.f, 1.f, 0.f);
    res.positions.emplace_back(-1.f, 1.f, 0.f);
    res.normals.resize(4, { 0.f, 0.f, 0.f });
    res.tangents.resize(4, { 0.f, 0.f, 0.f });

    res.uvs.emplace_back(0.f, 0.f);
    res.uvs.emplace_back(1.f, 0.f);
    res.uvs.emplace_back(1.f, 1.f);
    res.uvs.emplace_back(-0.f, 1.f);

    res.indexes.emplace_back(0, 1, 2);
    res.indexes.emplace_back(2, 3, 0);

    return res;
}