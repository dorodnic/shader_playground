#include "tube-shader.h"

tube_shader::tube_shader()
    : simple_shader(shader_program::load(
        "resources/shaders/tube/tube-vertex.glsl",
        "resources/shaders/tube/tube-fragment.glsl"))
{
    _shine2_location = _shader->get_uniform_location("shineDamper2");
    _reflectivity2_location = _shader->get_uniform_location("reflectivity2");
    _distortion_location = _shader->get_uniform_location("distortion");

    auto refractionSampler_location = _shader->get_uniform_location("refractionSampler");

    _shader->begin();
    _shader->load_uniform(refractionSampler_location, 2);
    _shader->end();
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