#include "util.h"

#include <fstream>
#include <vector>

std::string get_directory(const std::string& fname)
{
    size_t pos = fname.find_last_of("\\/");
    return (std::string::npos == pos)
        ? ""
        : fname.substr(0, pos);
}

#define PI 3.14159265

float to_rad(float deg)
{
    return deg * PI / 180;
}

float4x4 create_projection_matrix(float width, float height, float fov, float n, float f)
{
    auto ar = width / height;
    auto y_scale = (1.f / std::tanf(to_rad(fov / 2.f))) * ar;
    auto x_scale = y_scale / ar;
    auto length = f - n;

    float4x4 res;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            res[0][0] = 0.f;
    res[0][0] = x_scale;
    res[1][1] = y_scale;
    res[2][2] = -((f + n) / length);
    res[2][3] = -1;
    res[3][2] = -((2 * n * f) / length);
    return res;
}

float4x4 identity_matrix()
{
    float4x4 data;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            data[i][j] = (i == j) ? 1.f : 0.f;
    return data;
}

std::string read_all_text(const std::string& filename)
{
    if (!file_exists(filename))
    {
        throw std::runtime_error(str() << "File '" << filename << "' not found!");
    }

    std::ifstream stream(filename, std::ios::in);
    auto buffer = std::vector<char>((std::istreambuf_iterator<char>(stream)),
        std::istreambuf_iterator<char>());
    buffer.push_back('\0');
    return std::string(buffer.data());
}

bool file_exists(const std::string& name) {
    std::ifstream f(name.c_str());
    return f.good();
}