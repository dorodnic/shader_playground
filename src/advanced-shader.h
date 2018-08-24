#pragma once

#include "util.h"
#include "shader.h"

struct light
{
    float3 position;
    float3 colour;
};

class advanced_shader
{
public:
    advanced_shader();

    void begin();
    void end();

    void set_mvp(const float4x4& model,
                 const float4x4& view,
                 const float4x4& projection);

    void set_light(const light& l);

    void set_material_properties(float ambient,
        float shine, float reflectivity);

private:
    std::unique_ptr<shader_program> _shader;

    uint32_t _transformation_matrix_location;
    uint32_t _projection_matrix_location;
    uint32_t _camera_matrix_location;

    uint32_t _light_position_location;
    uint32_t _light_colour_location;

    uint32_t _diffuse_location;
    uint32_t _shine_location;
    uint32_t _reflectivity_location;
};