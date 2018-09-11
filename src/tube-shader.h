#pragma once

#include "util.h"
#include "simple-shader.h"

class tube_shader : public simple_shader
{
public:
    tube_shader();

    void enable_normal_mapping(bool enabled);

    void set_distortion(float d);

    void set_material_properties(float ambient,
        float shine, float reflectivity) override;

    void set_decal_uvs(const float2& uvs);
    void set_decal_id(int decal, int variations);

    int normal_map_slot() const { return 1; }
    int refraction_slot() const { return 2; }
    int decal_atlas_slot() const { return 3; }

private:
    uint32_t _shine2_location;
    uint32_t _reflectivity2_location;
    uint32_t _distortion_location;
    uint32_t _do_normal_mapping_location;
    uint32_t _decal_uvs_location;
    uint32_t _decal_id_location;
    uint32_t _decal_variations_locations;
};