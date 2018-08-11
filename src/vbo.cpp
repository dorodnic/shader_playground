#include "vbo.h"

#include <GL/glew.h>
#include <easylogging++.h>
#include <assert.h>

int vbo::convert_type(vbo_type type)
{
    switch (type) {
    case vbo_type::array_buffer: return GL_ARRAY_BUFFER;
    case vbo_type::element_array_buffer: return GL_ELEMENT_ARRAY_BUFFER;
    default: throw std::runtime_error("Not supported VBO type!");
    }
}

vbo::vbo(vbo_type type)
    : _type(type)
{
    glGenBuffers(1, &_id);
}

void vbo::bind()
{
    glBindBuffer(convert_type(_type), _id);
}

void vbo::unbind()
{
    glBindBuffer(convert_type(_type), 0);
}

void vbo::upload(int attribute, float* xyz, int size, int count)
{
    assert(_type == vbo_type::array_buffer);
    bind();
    glBufferData(convert_type(_type), count * size * sizeof(float), xyz, GL_STATIC_DRAW);
    glVertexAttribPointer(attribute, size, GL_FLOAT, GL_FALSE, 0, 0);
    _size = count;
    unbind();
}

void vbo::upload(int3* indx, int count)
{
    assert(_type == vbo_type::element_array_buffer);
    bind();
    glBufferData(convert_type(_type), count * sizeof(int3), indx, GL_STATIC_DRAW);
    _size = count;
}

void vbo::draw_triangles()
{
    assert(_type == vbo_type::array_buffer);
    bind();
    glDrawArrays(GL_TRIANGLES, 0, _size);
    unbind();
}

void vbo::draw_indexed_triangles()
{
    assert(_type == vbo_type::element_array_buffer);
    glDrawElements(GL_TRIANGLES, _size * (sizeof(int3) / sizeof(int)), GL_UNSIGNED_INT, 0);
}

vbo::~vbo()
{
    glDeleteBuffers(1, &_id);
}