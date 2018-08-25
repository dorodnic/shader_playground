#pragma once

#include <GL/gl3w.h>
#include "util.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

enum class shader_type
{
    vertex,
    fragment
};

class shader
{
public:
    shader(const std::string& filename, shader_type type);
    ~shader();
    
    unsigned int get_id() const { return _id; }
    
private:
    unsigned int _id;
};

class shader_program
{
public:
    shader_program();
    ~shader_program();
    
    void attach(const shader& shader);
    void link();
    
    void begin() const;
    void end() const;
    
    static std::unique_ptr<shader_program> load(
                            const std::string& vertex_shader,
                            const std::string& fragment_shader);
                              
    unsigned int get_id() const { return _id; }

    int get_uniform_location(const std::string& name);
    void load_uniform(int location, float value);
    void load_uniform(int location, const float2& vec);
    void load_uniform(int location, const float3& vec);
    void load_uniform(int location, bool value);
    void load_uniform(int location, int value);
    void load_uniform(int location, const float4x4& matrix);

    void bind_attribute(int attr, const std::string& name);

private:
    std::vector<const shader*> _shaders;
    unsigned int _id;
};