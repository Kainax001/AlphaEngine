#pragma once

#include "AlphaGraphic/scene/material/Material.h"
#include <unordered_map>
#include <memory>
#include <string>

namespace AG {

class MaterialInstance
{
public:
    MaterialInstance(std::shared_ptr<Material> base);

    void Bind()   const; // Base 적용 후 Override 덮어씌움
    void Unbind() const;

    void SetTexture(const std::string& name, std::shared_ptr<Texture> texture);
    void SetBool (const std::string& name, bool value);
    void SetInt  (const std::string& name, int value);
    void SetFloat(const std::string& name, float value);
    void SetVec3 (const std::string& name, const glm::vec3& value);
    void SetVec4 (const std::string& name, const glm::vec4& value);
    void SetMat4 (const std::string& name, const glm::mat4& value);

    std::shared_ptr<Material> GetBase() const { return m_Base; }

private:
    std::shared_ptr<Material> m_Base;

    std::unordered_map<std::string, std::shared_ptr<Texture>> m_Textures;
    std::unordered_map<std::string, bool>      m_Bools;
    std::unordered_map<std::string, int>       m_Ints;
    std::unordered_map<std::string, float>     m_Floats;
    std::unordered_map<std::string, glm::vec3> m_Vec3s;
    std::unordered_map<std::string, glm::vec4> m_Vec4s;
    std::unordered_map<std::string, glm::mat4> m_Mat4s;
};

} // namespace AG
