#include "AlphaGraphic/scene/material/MaterialInstance.h"

namespace AG {

MaterialInstance::MaterialInstance(std::shared_ptr<Material> base)
    : m_Base(base)
{
}

void MaterialInstance::Bind() const
{
    m_Base->Bind();

    auto shader = m_Base->GetShader();
    int slot = m_Base->GetTextureCount(); // Base 이후 슬롯부터 오버라이드 할당

    for (auto& [name, texture] : m_Textures)
    {
        texture->Bind(slot);
        shader->SetInt(name, slot);
        slot++;
    }

    for (auto& [name, value] : m_Bools)  shader->SetBool (name, value);
    for (auto& [name, value] : m_Ints)   shader->SetInt  (name, value);
    for (auto& [name, value] : m_Floats) shader->SetFloat(name, value);
    for (auto& [name, value] : m_Vec3s)  shader->SetVec3 (name, value);
    for (auto& [name, value] : m_Vec4s)  shader->SetVec4 (name, value);
    for (auto& [name, value] : m_Mat4s)  shader->SetMat4 (name, value);
}

void MaterialInstance::Unbind() const
{
    m_Base->Unbind();

    for (auto& [name, texture] : m_Textures)
        texture->Unbind();
}

void MaterialInstance::SetTexture(const std::string& name, std::shared_ptr<Texture> texture) { m_Textures[name] = texture; }
void MaterialInstance::SetBool (const std::string& name, bool value)          { m_Bools[name]   = value; }
void MaterialInstance::SetInt  (const std::string& name, int value)           { m_Ints[name]    = value; }
void MaterialInstance::SetFloat(const std::string& name, float value)         { m_Floats[name]  = value; }
void MaterialInstance::SetVec3 (const std::string& name, const glm::vec3& v)  { m_Vec3s[name]   = v; }
void MaterialInstance::SetVec4 (const std::string& name, const glm::vec4& v)  { m_Vec4s[name]   = v; }
void MaterialInstance::SetMat4 (const std::string& name, const glm::mat4& m)  { m_Mat4s[name]   = m; }

} // namespace AG
