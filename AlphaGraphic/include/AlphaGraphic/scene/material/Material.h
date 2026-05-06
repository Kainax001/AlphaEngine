#pragma once

#include "AlphaGraphic/shader/Shader.h"
#include "AlphaGraphic/texture/Texture.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

namespace AG {

class Material
{
public:
    Material(std::shared_ptr<Shader> shader);

    void Bind()   const;
    void Unbind() const;

    // --- Texture ---
    void SetTexture(const std::string& name, std::shared_ptr<Texture> texture);

    // --- Scalar / Vector ---
    void SetBool (const std::string& name, bool value);
    void SetInt  (const std::string& name, int value);
    void SetFloat(const std::string& name, float value);
    void SetVec3 (const std::string& name, const glm::vec3& value);
    void SetVec4 (const std::string& name, const glm::vec4& value);
    void SetMat4 (const std::string& name, const glm::mat4& value);

    // --- Getter ---
    bool      GetBool (const std::string& name) const;
    int       GetInt  (const std::string& name) const;
    float     GetFloat(const std::string& name) const;
    glm::vec3 GetVec3 (const std::string& name) const;
    glm::vec4 GetVec4 (const std::string& name) const;

    bool HasTexture (const std::string& name) const;
    bool HasProperty(const std::string& name) const;

    std::shared_ptr<Shader> GetShader()       const { return m_Shader; }
    int                     GetTextureCount() const { return m_NextSlot; }

private:
    struct TextureEntry {
        std::shared_ptr<Texture> texture;
        int slot;
    };

    std::shared_ptr<Shader> m_Shader;

    std::vector<std::pair<std::string, TextureEntry>> m_Textures; // 삽입 순서 = slot 번호 보장
    std::unordered_map<std::string, bool>      m_Bools;
    std::unordered_map<std::string, int>       m_Ints;
    std::unordered_map<std::string, float>     m_Floats;
    std::unordered_map<std::string, glm::vec3> m_Vec3s;
    std::unordered_map<std::string, glm::vec4> m_Vec4s;
    std::unordered_map<std::string, glm::mat4> m_Mat4s;

    int m_NextSlot = 0;
};

} // namespace AG
