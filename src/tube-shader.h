#pragma once

#include "util.h"
#include "simple-shader.h"

class tube_shader : public simple_shader
{
public:
    tube_shader();

    void set_distortion(float d);

    void set_material_properties(float ambient,
        float shine, float reflectivity) override;

private:
    uint32_t _shine2_location;
    uint32_t _reflectivity2_location;
    uint32_t _distortion_location;
};