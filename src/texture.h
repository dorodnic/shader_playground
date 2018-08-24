#pragma once

#include <string>

class texture
{
public:
    texture();
    ~texture();

    void upload(const std::string& filename);
    void upload(int channels, int bits_per_channel, int width, int height, uint8_t* data);

    void bind(int texture_slot) const;
    void unbind() const;

    uint32_t get() const { return _texture; }
private:
    uint32_t _texture;
};