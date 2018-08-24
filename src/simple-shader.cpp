#include "simple-shader.h"

simple_shader::simple_shader()
{
    _shader = shader_program::load(
        "resources/shaders/simple/simp-vertex.glsl",
        "resources/shaders/simple/simp-fragment.glsl");

    _shader->bind_attribute(0, "position");
    _shader->bind_attribute(1, "textureCoords");
    _shader->bind_attribute(2, "normal");
    _shader->bind_attribute(3, "tangent");

    _transformation_matrix_location = _shader->get_uniform_location("transformationMatrix");
    _projection_matrix_location = _shader->get_uniform_location("projectionMatrix");
    _camera_matrix_location = _shader->get_uniform_location("cameraMatrix");

    _light_position_location = _shader->get_uniform_location("lightPosition");

    _ambient_location = _shader->get_uniform_location("ambient");
    _shine_location = _shader->get_uniform_location("shineDamper");
    _reflectivity_location = _shader->get_uniform_location("reflectivity");

    auto texture0_sampler_location = _shader->get_uniform_location("textureSampler");

    _shader->begin();
    _shader->load_uniform(texture0_sampler_location, 0);
    _shader->end();
}

void simple_shader::begin() { _shader->begin(); }
void simple_shader::end() { _shader->end(); }

void simple_shader::set_mvp(const float4x4& model,
    const float4x4& view,
    const float4x4& projection)
{
    _shader->load_uniform(_transformation_matrix_location, model);
    _shader->load_uniform(_camera_matrix_location, view);
    _shader->load_uniform(_projection_matrix_location, projection);
}

void simple_shader::set_light(const float3& l)
{
    _shader->load_uniform(_light_position_location, l);
}

void simple_shader::set_material_properties(
    float ambient, float shine, float reflectivity)
{
    _shader->load_uniform(_shine_location, shine);
    _shader->load_uniform(_reflectivity_location, reflectivity);
    _shader->load_uniform(_ambient_location, ambient);
}