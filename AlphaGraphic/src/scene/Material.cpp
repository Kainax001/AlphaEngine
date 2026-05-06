#include "AlphaGraphic/scene/Material.h"

namespace AG {

Material::Material(std::shared_ptr<Shader> shader)
    : m_Shader(shader)
{
}

void Material::Bind() const
{
    m_Shader->Use();

    unsigned int slot = 0;
    for (auto& [name, texture] : m_Textures)
    {
        texture->Bind(slot);
        m_Shader->SetInt(name, (int)slot);
        slot++;
    }

    for (auto& [name, value] : m_Floats)
        m_Shader->SetFloat(name, value);

    for (auto& [name, value] : m_Vec3s)
        m_Shader->SetVec3(name, value);

    for (auto& [name, value] : m_Vec4s)
        m_Shader->SetVec4(name, value);
}

void Material::Unbind() const
{
    unsigned int slot = 0;
    for (auto& [name, texture] : m_Textures)
    {
        texture->Unbind();
        slot++;
    }
}

void Material::SetTexture(const std::string& name, std::shared_ptr<Texture> texture)
{
    m_Textures[name] = texture;
}

void Material::SetFloat(const std::string& name, float value)
{
    m_Floats[name] = value;
}

void Material::SetVec3(const std::string& name, const glm::vec3& value)
{
    m_Vec3s[name] = value;
}

void Material::SetVec4(const std::string& name, const glm::vec4& value)
{
    m_Vec4s[name] = value;
}

} // namespace AG