#pragma once

#include <string>

#include <sstream>

#include <linalg.h>

class util_exception : public std::runtime_error
{
public:
    util_exception(const std::string& err);
};

using namespace linalg::aliases;
using namespace linalg;

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

float4x4 create_perspective_projection_matrix(float width, float height, float fov, float n, float f);
float4x4 create_orthographic_projection_matrix(float width, float height, float fov, float n, float f);
float4x4 identity_matrix();

std::string read_all_text(const std::string& filename);
bool file_exists(const std::string& name);
std::string get_directory(const std::string& fname);