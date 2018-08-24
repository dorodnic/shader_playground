#pragma once

#include "util.h"
#include "texture.h"

class fbo
{
public:
    fbo(int w, int h);

    void createTextureAttachment();

    void createDepthTextureAttachment();

    void bind();

    void unbind();

    void createDepthBufferAttachment();

    ~fbo();

    texture& get_color_texture() { return _color_tex; }
    texture& get_depth_texture() { return _depth_tex; }

    const texture& get_color_texture() const { return _color_tex; }
    const texture& get_depth_texture() const { return _depth_tex; }

private:
    texture _color_tex;
    texture _depth_tex;
    uint32_t _id;
    uint32_t _db = 0;
    int _w, _h;
};