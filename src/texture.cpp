#include "texture.h"
#include "util.h"

#include <GL/gl3w.h>

#include <easylogging++.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void texture::bind(int texture_slot) const
{
    glActiveTexture(GL_TEXTURE0 + texture_slot);
    glBindTexture(GL_TEXTURE_2D, _texture);
}

void texture::set_options(bool linear, bool mipmap)
{
    _linear = linear;
    _mipmap = mipmap;
}

void texture::unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

texture::texture() : _texture()
{
    glGenTextures(1, &_texture);
}

texture::~texture()
{
    glDeleteTextures(1, &_texture);
}

void texture::upload(const std::string& filename)
{
    if (!file_exists(filename))
        throw std::runtime_error("Texture file not found!");

    int x, y, comp;
    auto r = stbi_load(filename.c_str(), &x, &y, &comp, false);
    upload(comp, 8, x, y, r);
    stbi_image_free(r);
}

void texture::upload(int channels, int bits_per_channel, int width, int height, uint8_t* data)
{
    bind(0);

    if (channels == 3 && bits_per_channel == 8)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    }
    else if (channels == 4 && bits_per_channel == 8)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
    else if (channels == 1 && bits_per_channel == 8)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
    }
    else if (channels == 1 && bits_per_channel == 16)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RED, GL_UNSIGNED_SHORT, data);
    }
    else if (channels == 1 && bits_per_channel == 32)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, data);
    }
    else throw std::runtime_error("Unsupported image format!");

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (!_mipmap)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _linear ? GL_LINEAR : GL_NEAREST);
    }
    else
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -1);
    }

    unbind();
}