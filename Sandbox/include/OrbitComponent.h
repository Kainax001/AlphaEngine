#pragma once
#include <AlphaScene/AlphaScene.h>
#include <glm/glm.hpp>
#include <cmath>
#include <typeindex>

// Moves the owner Actor in a horizontal circle every Tick.
// radius : orbit radius (world units)
// speed  : angular speed (radians / second)
// height : fixed Y position during orbit
class OrbitComponent : public AS::Component {
public:
    OrbitComponent(AS::Actor* owner,
                   float radius = 4.0f,
                   float speed  = 1.2f,
                   float height = 3.0f)
        : Component(owner)
        , m_Radius(radius)
        , m_Speed(speed)
        , m_Height(height)
        , m_Time(0.0f)
    {}

    void Tick(float dt) override
    {
        m_Time += dt;
        float ang = m_Time * m_Speed;
        m_Owner->GetTransform().SetPosition(
            glm::vec3(std::cos(ang) * m_Radius,
                      m_Height,
                      std::sin(ang) * m_Radius));
    }

    const char*     GetTypeName()  const override { return "OrbitComponent"; }
    std::type_index GetTypeIndex() const override { return typeid(OrbitComponent); }

private:
    float m_Radius;
    float m_Speed;
    float m_Height;
    float m_Time;
};
