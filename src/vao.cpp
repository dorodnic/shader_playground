#include "vao.h"

#include <easylogging++.h>

vao::vao(const float3* vert, const float2* uvs, const float3* normals,
         const float3* tangents, int vert_count, const int3* indx, int indx_count)
    : _vertexes(vbo_type::array_buffer),
      _uvs(vbo_type::array_buffer),
      _indexes(vbo_type::element_array_buffer),
      _tangents(vbo_type::array_buffer)
{
    glGenVertexArrays(1, &_id);
    bind();
    _indexes.upload(indx, indx_count);
    _vertexes.upload(0, (float*)vert, 3, vert_count);
    _normals.upload(2, (float*)normals, 3, vert_count);
    _tangents.upload(3, (float*)tangents, 3, vert_count);
    _uvs.upload(1, (float*)uvs, 2, vert_count);
    unbind();
}

vao::vao(vao&& other)
    : _id(other._id), 
      _indexes(std::move(other._indexes)),
      _vertexes(std::move(other._vertexes)),
      _uvs(std::move(other._uvs)),
      _normals(std::move(other._normals)),
      _tangents(std::move(other._tangents))
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

void vao::draw()
{
    bind();

    glEnableVertexAttribArray(0); // vertex
    glEnableVertexAttribArray(1); // uv
    glEnableVertexAttribArray(2); // normals
    glEnableVertexAttribArray(3); // tangents
    
    _indexes.draw_indexed_triangles();
    
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);

    unbind();
}