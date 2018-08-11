#pragma once

#include <string>

class texture
{
public:
    texture();
    ~texture();

    void upload(const std::string& filename);
    void upload(int channels, int bits_per_channel, int width, int height, int stride, uint8_t* data);

    void bind() const;
    void unbind() const;
private:
    uint32_t _texture;
};