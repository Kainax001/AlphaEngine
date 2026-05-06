#include "AlphaGraphic/scene/light/SpotLight.h"

namespace AG {

SpotLight::SpotLight(const glm::vec3& position, const glm::vec3& direction,
                     const glm::vec3& color, float intensity)
    : m_Position(position)
    , m_Direction(direction)
{
    m_Color     = color;
    m_Intensity = intensity;
}

} // namespace AG