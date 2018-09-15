#pragma once

#include "util.h"
#include "shader.h"
#include "vao.h"
#include "texture.h"

class texture_2d_shader
{
public:
    texture_2d_shader();

    void begin();
    void end();

    void set_position_and_scale(
        const float2& position,
        const float2& scale);

protected:
    texture_2d_shader(std::unique_ptr<shader_program> shader);

    std::unique_ptr<shader_program> _shader;

private:
    void init();

    uint32_t _position_location;
    uint32_t _scale_location;
};

class texture_visualizer
{
public:
    texture_visualizer(float2 pos, float2 scale)
        : _position(std::move(pos)),
          _scale(std::move(scale)),
          _geometry(vao::create(create_mesh()))
    {

    }

    texture_visualizer()
        : texture_visualizer({ 0.f, 0.f }, { 1.f, 1.f }) {}

    void set_position(float2 pos) { _position = pos; }
    void set_scale(float2 scale) { _scale = scale; }

    void draw(texture_2d_shader& shader, texture& tex);

private:
    static obj_mesh create_mesh();

    float2 _position;
    float2 _scale;
    std::shared_ptr<vao> _geometry;
};

class visualizer_2d
{
public:
    visualizer_2d()
        : visualizer_2d(std::make_shared<texture_2d_shader>())
    {
    }

    visualizer_2d(std::shared_ptr<texture_2d_shader> shader)
        : tex_2d_shader(shader)
    {
    }

    void draw_texture(texture& tex)
    {
        draw_texture({ 0.f, 0.f }, { 1.f, 1.f }, tex);
    }

    void draw_texture(float2 pos, float2 scale, texture& tex)
    {
        tex_2d_shader->begin();
        _visualizer.set_position(pos);
        _visualizer.set_scale(scale);
        _visualizer.draw(*tex_2d_shader, tex);
        tex_2d_shader->end();
    }
private:
    texture_visualizer _visualizer;
    std::shared_ptr<texture_2d_shader> tex_2d_shader;
};