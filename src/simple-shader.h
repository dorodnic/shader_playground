#pragma once

#include "util.h"
#include "shader.h"

class simple_shader
{
public:
    simple_shader();

    void begin();
    void end();

    void set_mvp(const float4x4& model,
                 const float4x4& view,
                 const float4x4& projection);

    void set_light(const float3& l);

    void set_material_properties(float ambient,
        float shine, float reflectivity);

private:
    std::unique_ptr<shader_program> _shader;

    uint32_t _transformation_matrix_location;
    uint32_t _projection_matrix_location;
    uint32_t _camera_matrix_location;

    uint32_t _light_position_location;

    uint32_t _ambient_location;
    uint32_t _shine_location;
    uint32_t _reflectivity_location;
};