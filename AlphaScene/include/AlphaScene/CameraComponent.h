#pragma once
#include "Component.h"
#include <AlphaGraphic/AlphaGraphic.h>

namespace AS {

class CameraComponent : public Component {
public:
    explicit CameraComponent(Actor* owner);

    // Processes keyboard/mouse input from InputComponent on the same Actor.
    void Tick(float dt) override;

    glm::mat4 GetViewMatrix()                    const;
    glm::mat4 GetProjectionMatrix(float aspect)  const;

    void  SetPosition(const glm::vec3& pos);
    void  SetSpeed(float speed);
    void  SetAspect(float aspect) { m_Aspect = aspect; }
    float GetAspect()       const { return m_Aspect; }

    const AG::Camera& GetCamera() const { return m_Camera; }
          AG::Camera& GetCamera()       { return m_Camera; }

    AG::CameraProxy ToProxy() const;

    const char*     GetTypeName()  const override { return "CameraComponent"; }
    std::type_index GetTypeIndex() const override { return typeid(CameraComponent); }
    void Serialize  (rapidjson::Value& out,
                     rapidjson::Document::AllocatorType& alloc) const override;
    void Deserialize(const rapidjson::Value& in) override;

private:
    AG::Camera m_Camera;
    float      m_Aspect = 16.0f / 9.0f;
};

} // namespace AS
