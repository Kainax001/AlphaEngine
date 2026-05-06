#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

namespace AG {

class Shader
{
public:
    unsigned int m_ID;

    // ===== 기본 생성자 =====
    // 셰이더를 나중에 초기화할 때 사용
    Shader() : m_ID(0) {}

    // ===== 매개변수 생성자 =====
    // 파일 경로를 받아서 셰이더 컴파일 및 링크
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);

    void Use() const;

    void SetBool (const std::string& name, bool value)  const;
    void SetInt  (const std::string& name, int value)   const;
    void SetFloat(const std::string& name, float value) const;

    void SetVec2(const std::string& name, const glm::vec2& v)                    const;
    void SetVec2(const std::string& name, float x, float y)                      const;
    void SetVec3(const std::string& name, const glm::vec3& v)                    const;
    void SetVec3(const std::string& name, float x, float y, float z)             const;
    void SetVec4(const std::string& name, const glm::vec4& v)                    const;
    void SetVec4(const std::string& name, float x, float y, float z, float w)    const;

    void SetMat2(const std::string& name, const glm::mat2& m) const;
    void SetMat3(const std::string& name, const glm::mat3& m) const;
    void SetMat4(const std::string& name, const glm::mat4& m) const;

private:
    void checkCompileErrors(unsigned int shader, const std::string& type);
};

} // namespace AG