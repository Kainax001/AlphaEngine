#include "AlphaGraphic/scene/Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace AG {

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : m_Position(position)
    , m_WorldUp(up)
    , m_Yaw(yaw)
    , m_Pitch(pitch)
{
    UpdateVectors();
}

glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(m_Position, m_Position + m_Front, m_Up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const
{
    return glm::perspective(glm::radians(m_Zoom), aspectRatio, m_Near, m_Far);
}

void Camera::ProcessKeyboard(CameraMovement direction, float deltaTime)
{
    float velocity = m_Speed * deltaTime;

    switch (direction)
    {
    case CameraMovement::Forward:  m_Position += m_Front   * velocity; break;
    case CameraMovement::Backward: m_Position -= m_Front   * velocity; break;
    case CameraMovement::Left:     m_Position -= m_Right   * velocity; break;
    case CameraMovement::Right:    m_Position += m_Right   * velocity; break;
    case CameraMovement::Up:       m_Position += m_WorldUp * velocity; break;
    case CameraMovement::Down:     m_Position -= m_WorldUp * velocity; break;
    }
}

void Camera::ProcessMouseMove(float xOffset, float yOffset, bool constrainPitch)
{
    m_Yaw   += xOffset * m_Sensitivity;
    m_Pitch += yOffset * m_Sensitivity;

    if (constrainPitch)
        m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);

    UpdateVectors();
}

void Camera::ProcessMouseScroll(float yOffset)
{
    m_Zoom = std::clamp(m_Zoom - yOffset, 1.0f, 90.0f);
}

void Camera::UpdateVectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.y = sin(glm::radians(m_Pitch));
    front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

    m_Front = glm::normalize(front);
    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    m_Up    = glm::normalize(glm::cross(m_Right, m_Front));
}

} // namespace AG