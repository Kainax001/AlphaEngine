#pragma once

#include "AlphaGraphic/scene/light/Light.h"

namespace AG {

class DirectionalLight : public Light
{
public:
    DirectionalLight();
    DirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity);

    LightType GetType() const override { return LightType::Directional; }

    const glm::vec3& GetDirection() const { return m_Direction; }
    void             SetDirection(const glm::vec3& direction) { m_Direction = direction; }

private:
    glm::vec3 m_Direction;
};

} // namespace AG