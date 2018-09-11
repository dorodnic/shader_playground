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

    int get_width() const { return _width; }
    int get_height() const { return _height; }
    int get_bytes() const { return _width*_height*_bpp; }
    float get_load_time() const { return _load_time; }

    void set_options(bool linear, bool mipmap);

private:
    uint32_t _texture;
    bool _mipmap = true;
    bool _linear = true;
    int _width = 0, _height = 0, _bpp = 0;
    float _load_time = 0.;
};