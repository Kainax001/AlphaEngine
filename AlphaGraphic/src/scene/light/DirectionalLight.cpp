#include "AlphaGraphic/scene/light/DirectionalLight.h"

namespace AG {

DirectionalLight::DirectionalLight()
    : m_Direction(glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f)))
{
}

DirectionalLight::DirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity)
    : m_Direction(glm::normalize(direction))
{
    m_Color     = color;
    m_Intensity = intensity;
}

} // namespace AG