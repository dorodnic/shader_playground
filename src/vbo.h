#pragma once

#include <GL/glew.h>
#include "util.h"

enum class vbo_type
{
    array_buffer,
    element_array_buffer,
};

class vbo
{
public:
    vbo(vbo_type type = vbo_type::array_buffer);
    ~vbo();

    void upload(int attribute, float* xyz, int size, int count);
    void upload(triangle* indx, int count);

    void draw_triangles();
    void draw_indexed_triangles();

    void bind();
    void unbind();

private:
    static int convert_type(vbo_type type);

    uint32_t _id;
    uint32_t _size = 0;
    vbo_type _type;
};