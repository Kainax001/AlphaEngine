#include "AlphaGraphic/scene/material/Material.h"

#include <algorithm>

namespace AG {

Material::Material(std::shared_ptr<Shader> shader)
    : m_Shader(shader)
{
}

void Material::Bind() const
{
    m_Shader->Use();

    for (auto& [name, entry] : m_Textures)
    {
        entry.texture->Bind(entry.slot);
        m_Shader->SetInt(name, entry.slot);
    }

    for (auto& [name, value] : m_Bools)
        m_Shader->SetBool(name, value);

    for (auto& [name, value] : m_Ints)
        m_Shader->SetInt(name, value);

    for (auto& [name, value] : m_Floats)
        m_Shader->SetFloat(name, value);

    for (auto& [name, value] : m_Vec3s)
        m_Shader->SetVec3(name, value);

    for (auto& [name, value] : m_Vec4s)
        m_Shader->SetVec4(name, value);

    for (auto& [name, value] : m_Mat4s)
        m_Shader->SetMat4(name, value);
}

void Material::Unbind() const
{
    for (auto& [name, entry] : m_Textures)
        entry.texture->Unbind();
}

void Material::SetTexture(const std::string& name, std::shared_ptr<Texture> texture)
{
    for (auto& [key, entry] : m_Textures)
    {
        if (key == name)
        {
            entry.texture = texture;
            return;
        }
    }
    TextureEntry entry;
    entry.texture = texture;
    entry.slot    = m_NextSlot++;
    m_Textures.push_back(std::make_pair(name, entry));
}

void Material::SetBool(const std::string& name, bool value)   { m_Bools[name]   = value; }
void Material::SetInt (const std::string& name, int value)    { m_Ints[name]    = value; }
void Material::SetFloat(const std::string& name, float value) { m_Floats[name]  = value; }
void Material::SetVec3(const std::string& name, const glm::vec3& value) { m_Vec3s[name] = value; }
void Material::SetVec4(const std::string& name, const glm::vec4& value) { m_Vec4s[name] = value; }
void Material::SetMat4(const std::string& name, const glm::mat4& value) { m_Mat4s[name] = value; }

bool Material::GetBool(const std::string& name) const
{
    auto it = m_Bools.find(name);
    return (it != m_Bools.end()) ? it->second : false;
}

int Material::GetInt(const std::string& name) const
{
    auto it = m_Ints.find(name);
    return (it != m_Ints.end()) ? it->second : 0;
}

float Material::GetFloat(const std::string& name) const
{
    auto it = m_Floats.find(name);
    return (it != m_Floats.end()) ? it->second : 0.0f;
}

glm::vec3 Material::GetVec3(const std::string& name) const
{
    auto it = m_Vec3s.find(name);
    return (it != m_Vec3s.end()) ? it->second : glm::vec3(0.0f);
}

glm::vec4 Material::GetVec4(const std::string& name) const
{
    auto it = m_Vec4s.find(name);
    return (it != m_Vec4s.end()) ? it->second : glm::vec4(0.0f);
}

bool Material::HasTexture(const std::string& name) const
{
    for (auto& [key, entry] : m_Textures)
        if (key == name) return true;
    return false;
}

bool Material::HasProperty(const std::string& name) const
{
    return m_Bools.count(name) || m_Ints.count(name) || m_Floats.count(name)
        || m_Vec3s.count(name) || m_Vec4s.count(name) || m_Mat4s.count(name);
}

} // namespace AG
