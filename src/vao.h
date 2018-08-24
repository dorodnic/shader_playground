#pragma once
#include <GL/gl3w.h>

#include "util.h"
#include "texture.h"
#include "vbo.h"
#include "loader.h"

class vao
{
public:
    static std::unique_ptr<vao> create(const obj_mesh& m);

    vao(const float3* vert, const float2* uvs, const float3* normals, 
        const float3* tangents, int vert_count, const int3* indx, int indx_count);
    ~vao();
    void bind();
    void unbind();
    void draw();

    vao(vao&& other);

private:
    vao(const vao& other) = delete;

    uint32_t _id;
    vbo _vertexes, _normals, _indexes, _uvs, _tangents;
};