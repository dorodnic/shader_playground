#include "shader.h"

#include <GL/glew.h>

#include <easylogging++.h>

struct str
{
    operator std::string() const
    {
        return _ss.str();
    }

    template<class T>
    str& operator<<(T&& t)
    {
        _ss << t;
        return *this;
    }

    std::ostringstream _ss;
};


inline std::string read_all_text(const std::string& filename)
{
    std::ifstream stream(filename, std::ios::in);
    if (!stream.is_open())
    {
        throw std::runtime_error(str() << "File '" << filename << "' not found!");
    }

    auto buffer = std::vector<char>((std::istreambuf_iterator<char>(stream)),
        std::istreambuf_iterator<char>());
    buffer.push_back('\0');
    return std::string(buffer.data());
}

shader::shader(const std::string& filename, shader_type type)
{
    auto lambda = [&](){
        switch(type)
        {
            case shader_type::vertex: return GL_VERTEX_SHADER;
            case shader_type::fragment: return GL_FRAGMENT_SHADER;
            default:
                throw std::runtime_error("Unknown shader type!");
        }
    };
    const auto gl_type = lambda();
    
    GLuint shader_id = glCreateShader(gl_type);

    auto shader_code = read_all_text(filename);
    LOG(INFO) << "Compiling shader " << filename << "...";
    
    char const * source_ptr = shader_code.c_str();
    int length = shader_code.size();
    glShaderSource(shader_id, 1, &source_ptr, &length);
    
    glCompileShader(shader_id);

    GLint result;
    int log_length;
    
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
    if ((result == GL_FALSE) && (log_length > 0)){
        std::vector<char> error_message(log_length+1);
        glGetShaderInfoLog(shader_id, log_length, NULL, &error_message[0]);
        std::string error(&error_message[0]);
        LOG(ERROR) << error;
        glDeleteShader(shader_id);
        throw std::runtime_error(error);
    }
    
    LOG(INFO) << "Shader " << filename << " [OK]";
    
    _id = shader_id;
}

shader::~shader()
{
    glDeleteShader(_id);
}

shader_program::shader_program()
{
    GLuint program_id = glCreateProgram();
    _id = program_id;
}
shader_program::~shader_program()
{
    glUseProgram(0);
	glDeleteProgram(_id);
}

void shader_program::attach(const shader& shader)
{
    _shaders.push_back(&shader);
}
void shader_program::link()
{
    for(auto ps : _shaders)
    {
        glAttachShader(_id, ps->get_id());
    }
    
    LOG(INFO) << "Linking the shader program...";
    glLinkProgram(_id);
    
    GLint result;
    int log_length;

    glGetProgramiv(_id, GL_LINK_STATUS, &result);
    glGetProgramiv(_id, GL_INFO_LOG_LENGTH, &log_length);
    if ((result == GL_FALSE) && (log_length > 0)){
        std::vector<char> error_message(log_length+1);
        glGetProgramInfoLog(_id, log_length, NULL, &error_message[0]);
        std::string error(&error_message[0]);
        LOG(ERROR) << error;
        for(auto ps : _shaders)
        {
            glDetachShader(_id, ps->get_id());
        }
        throw std::runtime_error(error);
    }
    
    LOG(INFO) << "Validating the shader program...";
    glValidateProgram(_id);

    glGetProgramiv(_id, GL_VALIDATE_STATUS, &result);
    glGetProgramiv(_id, GL_INFO_LOG_LENGTH, &log_length);
    if ((result == GL_FALSE) && (log_length > 0)){
        std::vector<char> error_message(log_length+1);
        glGetProgramInfoLog(_id, log_length, NULL, &error_message[0]);
        std::string error(&error_message[0]);
        LOG(ERROR) << error;
        for(auto ps : _shaders)
        {
            glDetachShader(_id, ps->get_id());
        }
        throw std::runtime_error(error);
    }
    
    LOG(INFO) << "Shader Program ready";
    
    for(auto ps : _shaders)
    {
        glDetachShader(_id, ps->get_id());
    }
    _shaders.clear();
}

void shader_program::begin() const
{
    glUseProgram(_id);
}
void shader_program::end() const
{
    glUseProgram(0);
}

std::unique_ptr<shader_program> shader_program::load(
                                const std::string& vertex_shader,
                                const std::string& fragment_shader)
{
    std::unique_ptr<shader_program> res(new shader_program());
    shader vertex(vertex_shader, shader_type::vertex);
    shader fragment(fragment_shader, shader_type::fragment);
    res->attach(vertex);
    res->attach(fragment);
    res->link();
    return std::move(res);
}
