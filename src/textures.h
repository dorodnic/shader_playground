#pragma once

#include <iostream>
#include <chrono>
#include <string>
#include <memory>
#include <map>
#include <vector>

#include "texture.h"


class texture_object
{
public:
    texture_object(std::string name, 
        std::string filename)
        : _name(std::move(name)), _filename(std::move(filename))
    {
        _tex = std::make_shared<texture>();
    }

    texture& get() { return *_tex; }

    const std::string& get_name() const { return _name; }

    void release() { _tex.reset(); }

    void reload()
    {
        _tex = std::make_shared<texture>();
        if (_filename != "")
        {
            _tex->set_options(_linear, _mipmap);
            _tex->upload(_filename);
        }
    }

    bool& linear() { return _linear; }
    bool& mipmap() { return _mipmap; }
    bool& is_open() { return _is_open; }
private:
    std::shared_ptr<texture> _tex;
    std::string _name, _filename = "";

    bool _linear = true, _mipmap = true, _is_open = false;
};

class texture_handle
{
public:
    texture_handle() {}
    texture_handle(uint32_t index) : index(index) {}
    uint32_t get_index() const { return index; }

private:
    uint32_t index;
};

class textures
{
public:
    texture_handle add_texture(std::string name, std::string filename)
    {
        tos.emplace_back(name, filename);
        return tos.size() - 1;
    }
    texture_handle add_texture(std::string name)
    {
        return add_texture(name, "");
    }

    template<class T>
    void with_texture(texture_handle tex_id, int slot, T action)
    {
        auto& tex = tos[tex_id.get_index()].get();
        tex.bind(slot);
        action();
        tex.unbind();
    }

    texture& get(texture_handle tex_id)
    {
        return tos[tex_id.get_index()].get();
    }

    void reload_all()
    {
        for (auto& to : tos)
            to.reload();
    }

    std::vector<texture_object>& get_texture_objects() { return tos; }
private:
    std::vector<texture_object> tos;
};