#include "AlphaGraphic/shader/ComputeShader.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

namespace AG {

ComputeShader::ComputeShader(const std::string& computePath)
{
    Compile(computePath);
}

ComputeShader::~ComputeShader()
{
    if (m_ID != 0)
        glDeleteProgram(m_ID);
}

bool ComputeShader::Reload()
{
    if (m_Path.empty()) return false;
    if (m_ID != 0)
    {
        glDeleteProgram(m_ID);
        m_ID = 0;
    }
    return Compile(m_Path);
}

bool ComputeShader::Compile(const std::string& path)
{
    m_Path = path;

    std::string   code;
    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try
    {
        file.open(path);
        std::stringstream ss;
        ss << file.rdbuf();
        file.close();
        code = ss.str();
    }
    catch (const std::ifstream::failure&)
    {
        std::cerr << "[ComputeShader] Cannot open: " << path << "\n";
        return false;
    }

    const char* src = code.c_str();

    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    CheckErrors(shader, "COMPUTE");

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        glDeleteShader(shader);
        return false;
    }

    m_ID = glCreateProgram();
    glAttachShader(m_ID, shader);
    glLinkProgram(m_ID);
    CheckErrors(m_ID, "PROGRAM");

    GLint linked = 0;
    glGetProgramiv(m_ID, GL_LINK_STATUS, &linked);
    glDeleteShader(shader);

    if (!linked)
    {
        glDeleteProgram(m_ID);
        m_ID = 0;
        return false;
    }

    std::cout << "[ComputeShader] Loaded: " << path << "\n";
    return true;
}

void ComputeShader::Use() const
{
    glUseProgram(m_ID);
}

void ComputeShader::Dispatch(GLuint x, GLuint y, GLuint z) const
{
    glDispatchCompute(x, y, z);
}

void ComputeShader::MemoryBarrier(GLbitfield barriers) const
{
    glMemoryBarrier(barriers);
}

void ComputeShader::SetInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(m_ID, name.c_str()), value);
}

void ComputeShader::SetFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(m_ID, name.c_str()), value);
}

void ComputeShader::SetVec2(const std::string& name, const glm::vec2& v) const
{
    glUniform2fv(glGetUniformLocation(m_ID, name.c_str()), 1, glm::value_ptr(v));
}

void ComputeShader::SetVec3(const std::string& name, const glm::vec3& v) const
{
    glUniform3fv(glGetUniformLocation(m_ID, name.c_str()), 1, glm::value_ptr(v));
}

void ComputeShader::CheckErrors(GLuint id, const std::string& type) const
{
    GLint  success = 0;
    GLchar log[1024];

    if (type != "PROGRAM")
    {
        glGetShaderiv(id, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(id, 1024, nullptr, log);
            std::cerr << "[ComputeShader] Compile error (" << type << "):\n"
                      << log << "\n";
        }
    }
    else
    {
        glGetProgramiv(id, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(id, 1024, nullptr, log);
            std::cerr << "[ComputeShader] Link error:\n" << log << "\n";
        }
    }
}

} // namespace AG
