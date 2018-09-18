#pragma once

#include "texture-2d-shader.h"
#include "simple-shader.h"
#include "fbo.h"
#include "procedural.h"
#include "textures.h"
#include "tube-shader.h"

class glass_decals_shader : public texture_2d_shader
{
public:
    glass_decals_shader();

    void set_width_height(bool horizontal, int w, int h);
private:
    uint32_t _width_location, _height_location, _horizontal_location;
};

class radial_shader : public texture_2d_shader
{
public:
    radial_shader();

    void set_decals_count(int count);
private:
    uint32_t _decals_count_location;
};

class normal_mapper_shader : public simple_shader
{
public:
    normal_mapper_shader();
};

class glass_atlas
{
public:
    void release();

    void generate_decals(texture_handle white);
    void draw_scatter(float t, tube_shader& shader);
    void prepare_decal(tube_shader& shader);

    const int glass_variations = 4;

    glass_atlas(textures& tex);

    void set_hitpoint(const obj_mesh& mesh, const float4x4& transform, 
        int idx, int glass);

    texture_handle diffuse() const { return diffuse_glass_atlas; }
    texture_handle outline() const { return final_glass_atlas; }
private:
    void reload_fbos();

    std::shared_ptr<fbo> glass_impact, glass_impact2, texture_atlas;
    std::vector<std::vector<glass_peice>> _glass_models;
    std::vector<std::vector<std::pair<std::shared_ptr<vao>, glass_peice>>> glasses;
    texture_handle temp_glass_atlas;
    texture_handle final_glass_atlas;
    texture_handle diffuse_glass_atlas;
    textures& _textures;
    int glass_id = 0;

    float4x4 tsp;
    float2 uvs;
};