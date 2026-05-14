#include "AlphaScene/LightComponent.h"
#include "AlphaScene/Actor.h"

namespace AS {

LightComponent::LightComponent(Actor* owner, std::shared_ptr<AG::Light> light)
    : Component(owner), m_Light(std::move(light))
{
}

LightData LightComponent::GetLightData() const
{
    LightData data;

    if (!m_Light) return data;

    data.color     = m_Light->GetColor();
    data.intensity = m_Light->GetIntensity();

    switch (m_Light->GetType())
    {
        case AG::LightType::Directional:
        {
            auto* dir = static_cast<AG::DirectionalLight*>(m_Light.get());
            data.type      = LightType::Directional;
            data.direction = dir->GetDirection();
            break;
        }
        case AG::LightType::Point:
        {
            auto* pt = static_cast<AG::PointLight*>(m_Light.get());
            data.type     = LightType::Point;
            data.position = m_Owner->GetTransform().GetPosition();
            data.constant  = pt->GetConstant();
            data.linear    = pt->GetLinear();
            data.quadratic = pt->GetQuadratic();
            break;
        }
        default:
            break;
    }

    return data;
}

} // namespace AS
