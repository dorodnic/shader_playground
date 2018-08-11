#pragma once

#include <string>

#include <sstream>

struct float3
{
    float x, y, z;
};

struct float2
{
    float u, v;
};

struct triangle
{
    int idx[3];
};

struct str
{
    operator std::string() const
    {
        return _ss.str();
    }

    template<class T> str& operator<<(T&& t)
    {
        _ss << t;
        return *this;
    }

    std::ostringstream _ss;
};


std::string read_all_text(const std::string& filename);


bool file_exists(const std::string& name);