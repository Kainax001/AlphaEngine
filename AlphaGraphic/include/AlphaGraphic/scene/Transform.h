#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace AG {

class Transform
{
public:
    Transform() = default;

    glm::mat4 GetModelMatrix() const;

    const glm::vec3& GetPosition() const { return m_Position; }
    const glm::quat& GetRotation() const { return m_Rotation; }
    const glm::vec3& GetScale()    const { return m_Scale; }

    void SetPosition(const glm::vec3& position) { m_Position = position; }
    void SetRotation(const glm::quat& rotation) { m_Rotation = rotation; }
    void SetScale   (const glm::vec3& scale)    { m_Scale    = scale; }

    void SetEulerAngles(const glm::vec3& eulerDegrees);

    void Translate(const glm::vec3& delta) { m_Position += delta; }
    void Rotate   (float angleDeg, const glm::vec3& axis);
    void Scale    (const glm::vec3& factor) { m_Scale *= factor; }

private:
    glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
    glm::quat m_Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // identity
    glm::vec3 m_Scale    = { 1.0f, 1.0f, 1.0f };
};

} // namespace AG