#include "util.h"

#include <fstream>
#include <vector>

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