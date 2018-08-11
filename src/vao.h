#pragma once
#include <GL/glew.h>

#include "util.h"
#include "texture.h"
#include "vbo.h"

class vao
{
public:
    vao(float3* vert, float2* uvs, int vert_count,
        int3* indx, int indx_count);
    ~vao();
    void bind();
    void unbind();
    void draw(const texture& tex);

    vao(vao&& other);

private:
    vao(const vao& other) = delete;

    uint32_t _id;
    vbo _vertexes;
    vbo _indexes;
    vbo _uvs;
};