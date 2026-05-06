#include "AlphaGraphic/scene/Transform.h"

#include <glm/gtc/matrix_transform.hpp>

namespace AG {

glm::mat4 Transform::GetModelMatrix() const
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, m_Position);
    model = model * glm::mat4_cast(m_Rotation);
    model = glm::scale(model, m_Scale);
    return model;
}

void Transform::SetEulerAngles(const glm::vec3& eulerDegrees)
{
    m_Rotation = glm::quat(glm::radians(eulerDegrees));
}

void Transform::Rotate(float angleDeg, const glm::vec3& axis)
{
    m_Rotation = glm::rotate(m_Rotation, glm::radians(angleDeg), axis);
}

} // namespace AG