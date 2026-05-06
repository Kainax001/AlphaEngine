#include "AlphaGraphic/shader/Shader.h"

#include <fstream>
#include <sstream>
#include <iostream>

namespace AG {

// ==============================================================================
// Constructor
// ==============================================================================
Shader::Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath)
{
    // ===== 경로 디버깅 출력 =====
    std::cout << "[Shader] Loading vertex: " << vertexPath << "\n";
    std::cout << "[Shader] Loading fragment: " << fragmentPath << "\n";
    if (geometryPath) {
        std::cout << "[Shader] Loading geometry: " << geometryPath << "\n";
    }

    std::string   vertexCode;
    std::string   fragmentCode;
    std::string   geometryCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    std::ifstream gShaderFile;

    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try
    {
        // ===== 정점 셰이더 읽기 =====
        vShaderFile.open(vertexPath);
        std::stringstream vShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        vShaderFile.close();
        vertexCode = vShaderStream.str();
        std::cout << "[Shader] Vertex shader loaded: " << vertexCode.length() << " bytes\n";

        // ===== 프래그먼트 셰이더 읽기 =====
        fShaderFile.open(fragmentPath);
        std::stringstream fShaderStream;
        fShaderStream << fShaderFile.rdbuf();
        fShaderFile.close();
        fragmentCode = fShaderStream.str();
        std::cout << "[Shader] Fragment shader loaded: " << fragmentCode.length() << " bytes\n";

        // ===== 기하학 셰이더 읽기 (선택사항) =====
        if (geometryPath != nullptr)
        {
            gShaderFile.open(geometryPath);
            std::stringstream gShaderStream;
            gShaderStream << gShaderFile.rdbuf();
            gShaderFile.close();
            geometryCode = gShaderStream.str();
            std::cout << "[Shader] Geometry shader loaded: " << geometryCode.length() << " bytes\n";
        }
    }
    catch (std::ifstream::failure& e)
    {
        std::cerr << "[AlphaGraphic] ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << "\n";
        std::cerr << "[AlphaGraphic] SHADER INITIALIZATION FAILED - ABORTING\n";
        m_ID = 0;  // 셰이더 프로그램을 0으로 설정 (유효하지 않은 상태)
        return;    // ← 중요: 여기서 반환! 빈 코드로 진행하면 안 됨
    }

    // ===== OpenGL context 및 GLAD 확인 =====
    if (!glCreateShader) {
        std::cerr << "[AlphaGraphic] ERROR::GLAD_NOT_LOADED: glCreateShader is NULL\n";
        std::cerr << "[AlphaGraphic] OpenGL functions not initialized yet!\n";
        m_ID = 0;
        return;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    unsigned int vertex, fragment;

    // ===== 정점 셰이더 컴파일 =====
    std::cout << "[Shader] Creating vertex shader...\n";
    vertex = glCreateShader(GL_VERTEX_SHADER);
    if (vertex == 0) {
        std::cerr << "[AlphaGraphic] ERROR::glCreateShader(GL_VERTEX_SHADER) returned 0\n";
        std::cerr << "[AlphaGraphic] OpenGL context may not be active\n";
        m_ID = 0;
        return;
    }
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");

    // ===== 프래그먼트 셰이더 컴파일 =====
    std::cout << "[Shader] Creating fragment shader...\n";
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    if (fragment == 0) {
        std::cerr << "[AlphaGraphic] ERROR::glCreateShader(GL_FRAGMENT_SHADER) returned 0\n";
        m_ID = 0;
        return;
    }
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    // ===== 기하학 셰이더 컴파일 (선택사항) =====
    unsigned int geometry = 0;
    if (geometryPath != nullptr)
    {
        const char* gShaderCode = geometryCode.c_str();
        std::cout << "[Shader] Creating geometry shader...\n";
        geometry = glCreateShader(GL_GEOMETRY_SHADER);
        if (geometry == 0) {
            std::cerr << "[AlphaGraphic] ERROR::glCreateShader(GL_GEOMETRY_SHADER) returned 0\n";
            m_ID = 0;
            glDeleteShader(vertex);
            glDeleteShader(fragment);
            return;
        }
        glShaderSource(geometry, 1, &gShaderCode, NULL);
        glCompileShader(geometry);
        checkCompileErrors(geometry, "GEOMETRY");
    }

    // ===== 셰이더 프로그램 링크 =====
    std::cout << "[Shader] Creating and linking shader program...\n";
    m_ID = glCreateProgram();
    if (m_ID == 0) {
        std::cerr << "[AlphaGraphic] ERROR::glCreateProgram() returned 0\n";
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (geometryPath != nullptr)
            glDeleteShader(geometry);
        return;
    }

    glAttachShader(m_ID, vertex);
    glAttachShader(m_ID, fragment);
    if (geometryPath != nullptr)
        glAttachShader(m_ID, geometry);

    glLinkProgram(m_ID);
    checkCompileErrors(m_ID, "PROGRAM");

    // ===== 임시 셰이더 객체 삭제 (프로그램에 이미 포함됨) =====
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    if (geometryPath != nullptr)
        glDeleteShader(geometry);

    std::cout << "[Shader] Shader program created successfully (ID: " << m_ID << ")\n";
}

// ==============================================================================
// Use
// ==============================================================================
void Shader::Use() const
{
    glUseProgram(m_ID);
}

// ==============================================================================
// Uniform setters
// ==============================================================================
void Shader::SetBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(m_ID, name.c_str()), (int)value);
}

void Shader::SetInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(m_ID, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(m_ID, name.c_str()), value);
}

void Shader::SetVec2(const std::string& name, const glm::vec2& v) const
{
    glUniform2fv(glGetUniformLocation(m_ID, name.c_str()), 1, &v[0]);
}
void Shader::SetVec2(const std::string& name, float x, float y) const
{
    glUniform2f(glGetUniformLocation(m_ID, name.c_str()), x, y);
}

void Shader::SetVec3(const std::string& name, const glm::vec3& v) const
{
    glUniform3fv(glGetUniformLocation(m_ID, name.c_str()), 1, &v[0]);
}
void Shader::SetVec3(const std::string& name, float x, float y, float z) const
{
    glUniform3f(glGetUniformLocation(m_ID, name.c_str()), x, y, z);
}

void Shader::SetVec4(const std::string& name, const glm::vec4& v) const
{
    glUniform4fv(glGetUniformLocation(m_ID, name.c_str()), 1, &v[0]);
}
void Shader::SetVec4(const std::string& name, float x, float y, float z, float w) const
{
    glUniform4f(glGetUniformLocation(m_ID, name.c_str()), x, y, z, w);
}

void Shader::SetMat2(const std::string& name, const glm::mat2& m) const
{
    glUniformMatrix2fv(glGetUniformLocation(m_ID, name.c_str()), 1, GL_FALSE, &m[0][0]);
}

void Shader::SetMat3(const std::string& name, const glm::mat3& m) const
{
    glUniformMatrix3fv(glGetUniformLocation(m_ID, name.c_str()), 1, GL_FALSE, &m[0][0]);
}

void Shader::SetMat4(const std::string& name, const glm::mat4& m) const
{
    glUniformMatrix4fv(glGetUniformLocation(m_ID, name.c_str()), 1, GL_FALSE, &m[0][0]);
}

// ==============================================================================
// Private
// ==============================================================================
void Shader::checkCompileErrors(unsigned int shader, const std::string& type)
{
    GLint  success;
    GLchar infoLog[1024];

    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "[AlphaGraphic] ERROR::SHADER_COMPILATION_ERROR of type: "
                      << type << "\n" << infoLog << "\n";
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "[AlphaGraphic] ERROR::PROGRAM_LINKING_ERROR of type: "
                      << type << "\n" << infoLog << "\n";
        }
    }
}

} // namespace AG