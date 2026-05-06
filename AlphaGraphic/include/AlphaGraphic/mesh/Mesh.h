#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

namespace AG {

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

class Mesh
{
public:
    virtual ~Mesh() = default;

    virtual void Draw()   const = 0;
    virtual void Bind()   const = 0;
    virtual void Unbind() const = 0;

    virtual unsigned int GetVAO() const = 0;
    unsigned int GetVertexCount() const { return m_VertexCount; }
    unsigned int GetIndexCount()  const { return m_IndexCount; }

protected:
    unsigned int m_VertexCount = 0;
    unsigned int m_IndexCount  = 0;
};

} // namespace AG