#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

namespace AG {

class ComputeShader
{
public:
    ComputeShader() = default;
    explicit ComputeShader(const std::string& computePath);
    ~ComputeShader();

    void Use()    const;
    bool Reload();
    bool IsValid() const { return m_ID != 0; }

    // x,y,z: number of work groups (divide resolution by local_size)
    void Dispatch(GLuint x, GLuint y, GLuint z = 1) const;

    // Insert memory barrier after dispatch so the next stage can read the output.
    // Default: image access barrier (compute -> fragment/texture read)
    void MemoryBarrier(GLbitfield barriers = GL_SHADER_IMAGE_ACCESS_BARRIER_BIT) const;

    void SetInt  (const std::string& name, int value)          const;
    void SetFloat(const std::string& name, float value)        const;
    void SetVec2 (const std::string& name, const glm::vec2& v) const;
    void SetVec3 (const std::string& name, const glm::vec3& v) const;

    const std::string& GetPath() const { return m_Path; }

private:
    GLuint      m_ID = 0;
    std::string m_Path;

    bool Compile(const std::string& path);
    void CheckErrors(GLuint shader, const std::string& type) const;
};

} // namespace AG
