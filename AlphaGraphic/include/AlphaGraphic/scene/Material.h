#pragma once

#include "AlphaGraphic/shader/Shader.h"
#include "AlphaGraphic/texture/Texture.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include <memory>
#include <string>

namespace AG {

class Material
{
public:
    Material(std::shared_ptr<Shader> shader);

    void Bind()   const;
    void Unbind() const;

    void SetTexture(const std::string& name, std::shared_ptr<Texture> texture);
    void SetFloat  (const std::string& name, float value);
    void SetVec3   (const std::string& name, const glm::vec3& value);
    void SetVec4   (const std::string& name, const glm::vec4& value);

    std::shared_ptr<Shader> GetShader() const { return m_Shader; }

private:
    std::shared_ptr<Shader>  m_Shader;

    std::unordered_map<std::string, std::shared_ptr<Texture>> m_Textures;
    std::unordered_map<std::string, float>                    m_Floats;
    std::unordered_map<std::string, glm::vec3>                m_Vec3s;
    std::unordered_map<std::string, glm::vec4>                m_Vec4s;
};

} // namespace AG