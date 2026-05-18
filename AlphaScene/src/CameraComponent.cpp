#include "AlphaScene/CameraComponent.h"
#include "AlphaScene/InputComponent.h"
#include "AlphaScene/Actor.h"
#include <glm/gtc/matrix_inverse.hpp>

namespace AS {

CameraComponent::CameraComponent(Actor* owner)
    : Component(owner)
{
}

void CameraComponent::Tick(float dt)
{
    auto* input = m_Owner->GetComponent<InputComponent>();
    if (!input) return;

    const AC::Keyboard& kb    = input->GetKeyboard();
    const AC::Mouse&    mouse = input->GetMouse();

    if (kb.IsKeyDown(AC::Key::W)) m_Camera.ProcessKeyboard(AG::CameraMovement::Forward,  dt);
    if (kb.IsKeyDown(AC::Key::S)) m_Camera.ProcessKeyboard(AG::CameraMovement::Backward, dt);
    if (kb.IsKeyDown(AC::Key::A)) m_Camera.ProcessKeyboard(AG::CameraMovement::Left,     dt);
    if (kb.IsKeyDown(AC::Key::D)) m_Camera.ProcessKeyboard(AG::CameraMovement::Right,    dt);
    if (kb.IsKeyDown(AC::Key::E)) m_Camera.ProcessKeyboard(AG::CameraMovement::Up,       dt);
    if (kb.IsKeyDown(AC::Key::Q)) m_Camera.ProcessKeyboard(AG::CameraMovement::Down,     dt);

    m_Camera.ProcessMouseMove(mouse.GetDeltaX(), mouse.GetDeltaY());
    m_Camera.ProcessMouseScroll(mouse.GetScrollDelta());

    // Sync Actor Transform position with camera position
    m_Owner->GetTransform().SetPosition(m_Camera.GetPosition());
}

glm::mat4 CameraComponent::GetViewMatrix() const
{
    return m_Camera.GetViewMatrix();
}

glm::mat4 CameraComponent::GetProjectionMatrix(float aspect) const
{
    return m_Camera.GetProjectionMatrix(aspect);
}

void CameraComponent::SetPosition(const glm::vec3& pos)
{
    m_Camera.SetPosition(pos);
    m_Owner->GetTransform().SetPosition(pos);
}

void CameraComponent::SetSpeed(float speed)
{
    m_Camera.SetSpeed(speed);
}

void CameraComponent::Serialize(rapidjson::Value& out,
                                  rapidjson::Document::AllocatorType& alloc) const
{
    out.AddMember("speed", m_Camera.GetSpeed(), alloc);
}

void CameraComponent::Deserialize(const rapidjson::Value& in)
{
    if (in.HasMember("speed") && in["speed"].IsNumber())
        m_Camera.SetSpeed(in["speed"].GetFloat());
}

AG::CameraProxy CameraComponent::ToProxy() const
{
    AG::CameraProxy proxy;
    proxy.view           = GetViewMatrix();
    proxy.projection     = GetProjectionMatrix(m_Aspect);
    proxy.invView        = glm::inverse(proxy.view);
    proxy.invProjection  = glm::inverse(proxy.projection);
    proxy.position       = glm::vec4(m_Camera.GetPosition(), 1.0f);
    return proxy;
}

} // namespace AS
