#include "tube-shader.h"

tube_shader::tube_shader()
    : simple_shader(shader_program::load(
        "resources/shaders/tube/tube-vertex.glsl",
        "resources/shaders/tube/tube-fragment.glsl"))
{
    _shine2_location = _shader->get_uniform_location("shineDamper2");
    _reflectivity2_location = _shader->get_uniform_location("reflectivity2");
    _distortion_location = _shader->get_uniform_location("distortion");
    _do_normal_mapping_location = _shader->get_uniform_location("do_normal_mapping");
    _decal_uvs_location = _shader->get_uniform_location("decal_uvs");
    _decal_id_location = _shader->get_uniform_location("decal_id");
    _decal_variations_locations = _shader->get_uniform_location("decal_variations");

    auto refractionSampler_location = _shader->get_uniform_location("refractionSampler");
    auto destructionSample_location = _shader->get_uniform_location("destructionSampler");

    _shader->begin();
    _shader->load_uniform(refractionSampler_location, 2);
    _shader->load_uniform(destructionSample_location, 3);
    _shader->end();
}

void tube_shader::enable_normal_mapping(bool enabled)
{
    _shader->load_uniform(_do_normal_mapping_location, enabled ? 1.f : 0.f);
}

void tube_shader::set_distortion(float d)
{
    _shader->load_uniform(_distortion_location, d);
}

void tube_shader::set_material_properties(
    float ambient, float shine, float reflectivity)
{
    simple_shader::set_material_properties(ambient, shine, reflectivity);
    _shader->load_uniform(_shine2_location, shine * 2);
    _shader->load_uniform(_reflectivity2_location, reflectivity * 2);
}

void tube_shader::set_decal_uvs(const float2 & uvs)
{
    _shader->load_uniform(_decal_uvs_location, uvs);
}

void tube_shader::set_decal_id(int decal, int variations)
{
    _shader->load_uniform(_decal_id_location, decal);
    _shader->load_uniform(_decal_variations_locations, variations);
}
