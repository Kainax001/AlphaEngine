#include "AlphaGraphic/buffer/VBO.h"

namespace AG {

VBO::VBO(std::vector<Vertex>& vertices)
{
    glGenBuffers(1, &m_ID);
    glBindBuffer(GL_ARRAY_BUFFER, m_ID);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
}

VBO::~VBO()
{
    Delete();
}

void VBO::Bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, m_ID);
}

void VBO::Unbind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VBO::Delete()
{
    if (m_ID != 0)
    {
        glDeleteBuffers(1, &m_ID);
        m_ID = 0;
    }
}

} // namespace AG