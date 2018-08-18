#pragma once

#include "vao.h"
#include "loader.h"

#include <memory>

class model
{
public:
    std::string id;
    bool visible = true; 

    void create(obj_file& loader);
    model(std::string id) : id(id) {}

    float get_scale() const { return _max; }
    int vertex_count() const { return _vertex_count; }

    void render();

private:
    std::shared_ptr<vao> _geometry;
    float _max = 0.f;
    int _vertex_count = 0;
};