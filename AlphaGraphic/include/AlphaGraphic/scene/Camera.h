#pragma once

#include <glm/glm.hpp>

namespace AG {

enum class CameraMovement
{
    Forward,
    Backward,
    Left,
    Right,
    Up,
    Down,
};

class Camera
{
public:
    Camera(glm::vec3 position = { 0.0f, 0.0f, 3.0f },
           glm::vec3 up       = { 0.0f, 1.0f, 0.0f },
           float yaw = -90.0f, float pitch = 0.0f);

    glm::mat4 GetViewMatrix()       const;
    glm::mat4 GetProjectionMatrix(float aspectRatio) const;

    void ProcessKeyboard   (CameraMovement direction, float deltaTime);
    void ProcessMouseMove  (float xOffset, float yOffset, bool constrainPitch = true);
    void ProcessMouseScroll(float yOffset);

    const glm::vec3& GetPosition() const { return m_Position; }
    const glm::vec3& GetFront()    const { return m_Front; }
    float            GetZoom()     const { return m_Zoom; }

    void  SetPosition(const glm::vec3& position) { m_Position = position; }
    void  SetSpeed   (float speed)              { m_Speed    = speed; }
    float GetSpeed   ()               const     { return m_Speed; }

private:
    void UpdateVectors();

    glm::vec3 m_Position;
    glm::vec3 m_Front   = { 0.0f, 0.0f, -1.0f };
    glm::vec3 m_Up;
    glm::vec3 m_Right;
    glm::vec3 m_WorldUp;

    float m_Yaw   = -90.0f;
    float m_Pitch =   0.0f;
    float m_Speed       = 2.5f;
    float m_Sensitivity = 0.1f;
    float m_Zoom        = 45.0f;
    float m_Near        = 0.1f;
    float m_Far         = 100.0f;
};

} // namespace AG