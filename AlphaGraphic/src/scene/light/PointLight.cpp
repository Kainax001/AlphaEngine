#include "AlphaGraphic/scene/light/PointLight.h"

namespace AG {

PointLight::PointLight(const glm::vec3& position, const glm::vec3& color, float intensity)
    : m_Position(position)
{
    m_Color     = color;
    m_Intensity = intensity;
}

void PointLight::SetAttenuation(float constant, float linear, float quadratic)
{
    m_Constant  = constant;
    m_Linear    = linear;
    m_Quadratic = quadratic;
}

} // namespace AG