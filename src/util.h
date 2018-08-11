#pragma once

#include <string>

#include <sstream>

#include <linalg.h>

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


std::string read_all_text(const std::string& filename);


bool file_exists(const std::string& name);