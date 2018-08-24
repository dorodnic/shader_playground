#include "advanced-shader.h"

advanced_shader::advanced_shader()
{
    _shader = shader_program::load(
        "resources/shaders/vertex.glsl",
        "resources/shaders/fragment.glsl");

    _shader->bind_attribute(0, "position");
    _shader->bind_attribute(1, "textureCoords");
    _shader->bind_attribute(2, "normal");
    _shader->bind_attribute(3, "tangent");

    _transformation_matrix_location = _shader->get_uniform_location("transformationMatrix");
    _projection_matrix_location = _shader->get_uniform_location("projectionMatrix");
    _camera_matrix_location = _shader->get_uniform_location("cameraMatrix");

    _light_position_location = _shader->get_uniform_location("lightPosition");
    _light_colour_location = _shader->get_uniform_location("lightColour");

    _diffuse_location = _shader->get_uniform_location("diffuse");
    _shine_location = _shader->get_uniform_location("shineDamper");
    _reflectivity_location = _shader->get_uniform_location("reflectivity");

    auto texture0_sampler_location = _shader->get_uniform_location("textureSampler");
    auto texture1_sampler_location = _shader->get_uniform_location("textureDarkSampler");
    auto texture_normal_sampler_location = _shader->get_uniform_location("textureNormalSampler");
    auto texture_mask_sampler_location = _shader->get_uniform_location("textureMaskSampler");

    _shader->begin();
    _shader->load_uniform(texture0_sampler_location, 0);
    _shader->load_uniform(texture1_sampler_location, 1);
    _shader->load_uniform(texture_normal_sampler_location, 2);
    _shader->load_uniform(texture_mask_sampler_location, 3);
    _shader->end();
}

void advanced_shader::begin() { _shader->begin(); }
void advanced_shader::end() { _shader->end(); }

void advanced_shader::set_mvp(const float4x4& model,
    const float4x4& view,
    const float4x4& projection)
{
    _shader->load_uniform(_transformation_matrix_location, model);
    _shader->load_uniform(_camera_matrix_location, view);
    _shader->load_uniform(_projection_matrix_location, projection);
}

void advanced_shader::set_light(const light& l)
{
    _shader->load_uniform(_light_position_location, l.position);
    _shader->load_uniform(_light_colour_location, l.colour);
}

void advanced_shader::set_material_properties(
    float ambient, float shine, float reflectivity)
{
    _shader->load_uniform(_shine_location, shine);
    _shader->load_uniform(_reflectivity_location, reflectivity);
    _shader->load_uniform(_diffuse_location, ambient);
}