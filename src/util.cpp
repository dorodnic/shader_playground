#include "util.h"

#include <fstream>
#include <vector>

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