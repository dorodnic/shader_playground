#include "vao.h"

#include <easylogging++.h>

vao::vao(float3* vert, float2* uvs, float3* normals, int vert_count,
         int3* indx, int indx_count)
    : _vertexes(vbo_type::array_buffer),
      _uvs(vbo_type::array_buffer),
      _indexes(vbo_type::element_array_buffer)
{
    glGenVertexArrays(1, &_id);
    bind();
    _indexes.upload(indx, indx_count);
    _vertexes.upload(0, (float*)vert, 3, vert_count);
    _normals.upload(2, (float*)normals, 3, vert_count);
    _uvs.upload(1, (float*)uvs, 2, vert_count);
    unbind();
}

vao::vao(vao&& other)
    : _id(other._id), 
      _indexes(std::move(other._indexes)),
      _vertexes(std::move(other._vertexes)),
      _uvs(std::move(other._uvs)),
      _normals(std::move(other._normals))
{
    other._id = 0;
}

vao::~vao()
{
    if (_id) glDeleteVertexArrays(1, &_id);
}

void vao::bind()
{
    glBindVertexArray(_id);
}

void vao::unbind()
{
    glBindVertexArray(0);
}

void vao::draw(const texture& tex)
{
    bind();

    glEnableVertexAttribArray(0); // vertex
    glEnableVertexAttribArray(1); // uv
    glEnableVertexAttribArray(2); // normals
    glActiveTexture(GL_TEXTURE0);
    tex.bind();
    _indexes.draw_indexed_triangles();
    tex.unbind();
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    unbind();
}