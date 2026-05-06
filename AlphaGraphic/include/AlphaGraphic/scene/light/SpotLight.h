#pragma once

#include "AlphaGraphic/scene/light/Light.h"

namespace AG {

class SpotLight : public Light
{
public:
    SpotLight() = default;
    SpotLight(const glm::vec3& position, const glm::vec3& direction,
              const glm::vec3& color, float intensity);

    LightType GetType() const override { return LightType::Spot; }

    const glm::vec3& GetPosition()  const { return m_Position; }
    const glm::vec3& GetDirection() const { return m_Direction; }
    float            GetCutOff()    const { return m_CutOff; }
    float            GetOuterCutOff() const { return m_OuterCutOff; }

    void SetPosition   (const glm::vec3& position)  { m_Position  = position; }
    void SetDirection  (const glm::vec3& direction) { m_Direction = direction; }
    void SetCutOff     (float cutOff)               { m_CutOff      = cutOff; }
    void SetOuterCutOff(float outerCutOff)          { m_OuterCutOff = outerCutOff; }

private:
    glm::vec3 m_Position    = { 0.0f, 0.0f, 0.0f };
    glm::vec3 m_Direction   = { 0.0f, -1.0f, 0.0f };
    float     m_CutOff      = 12.5f; // degrees
    float     m_OuterCutOff = 17.5f;
};

} // namespace AG