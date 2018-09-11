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

inline std::string bytes_to_string(long bytes)
{
    std::stringstream ss;
    unsigned long base = 2;
    if (bytes < (base << 10))
        ss << bytes << " bytes";
    else  if (bytes < (base << 20))
        ss << (bytes >> 10) << " Kb";
    else  if (bytes < (base << 30))
        ss << (bytes >> 20) << " Mb";
    else
        ss << (bytes >> 30) << " Gb";
    return ss.str();
}

float4x4 create_perspective_projection_matrix(float width, float height, float fov, float n, float f);
float4x4 create_orthographic_projection_matrix(float width, float height, float fov, float n, float f);
float4x4 identity_matrix();

std::string read_all_text(const std::string& filename);
bool file_exists(const std::string& name);
std::string get_directory(const std::string& fname);