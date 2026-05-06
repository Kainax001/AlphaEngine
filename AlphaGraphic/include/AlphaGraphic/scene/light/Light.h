#pragma once

#include <glm/glm.hpp>

namespace AG {

enum class LightType
{
    Directional,
    Point,
    Spot,
};

class Light
{
public:
    virtual ~Light() = default;

    virtual LightType GetType() const = 0;

    const glm::vec3& GetColor()    const { return m_Color; }
    float            GetIntensity() const { return m_Intensity; }

    void SetColor    (const glm::vec3& color) { m_Color     = color; }
    void SetIntensity(float intensity)        { m_Intensity = intensity; }

protected:
    glm::vec3 m_Color     = { 1.0f, 1.0f, 1.0f };
    float     m_Intensity = 1.0f;
};

} // namespace AG