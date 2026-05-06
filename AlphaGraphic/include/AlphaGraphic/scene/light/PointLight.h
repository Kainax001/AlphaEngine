#pragma once

#include "AlphaGraphic/scene/light/Light.h"

namespace AG {

class PointLight : public Light
{
public:
    PointLight() = default;
    PointLight(const glm::vec3& position, const glm::vec3& color, float intensity);

    LightType GetType() const override { return LightType::Point; }

    const glm::vec3& GetPosition() const { return m_Position; }
    float            GetConstant()  const { return m_Constant; }
    float            GetLinear()    const { return m_Linear; }
    float            GetQuadratic() const { return m_Quadratic; }

    void SetPosition (const glm::vec3& position) { m_Position  = position; }
    void SetAttenuation(float constant, float linear, float quadratic);

private:
    glm::vec3 m_Position  = { 0.0f, 0.0f, 0.0f };
    float     m_Constant  = 1.0f;
    float     m_Linear    = 0.09f;
    float     m_Quadratic = 0.032f;
};

} // namespace AG