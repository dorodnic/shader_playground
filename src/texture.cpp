#include "texture.h"
#include "util.h"

#include <GL/glew.h>

#include <easylogging++.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void texture::bind() const
{
    glBindTexture(GL_TEXTURE_2D, _texture);
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
    upload(comp, 8, x, y, x, r);
    stbi_image_free(r);
}

void texture::upload(int channels, int bits_per_channel, int width, int height, int stride, uint8_t* data)
{
    bind();

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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
    }
    else if (channels == 1 && bits_per_channel == 16)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, data);
    }
    else throw std::runtime_error("Unsupported image format!");

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    
    unbind();
}