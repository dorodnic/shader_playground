#pragma once

#include "util.h"
#include "shader.h"

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