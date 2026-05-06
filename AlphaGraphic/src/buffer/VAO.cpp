#include "AlphaGraphic/buffer/VAO.h"

namespace AG {

VAO::VAO()
{
    glGenVertexArrays(1, &m_ID);
}

VAO::~VAO()
{
    Delete();
}

void VAO::LinkAttrib(VBO& vbo, GLuint layout, GLuint numComponents,
                     GLenum type, GLsizeiptr stride, void* offset)
{
    vbo.Bind();
    glVertexAttribPointer(layout, numComponents, type, GL_FALSE, stride, offset);
    glEnableVertexAttribArray(layout);
    vbo.Unbind();
}

void VAO::Bind() const
{
    glBindVertexArray(m_ID);
}

void VAO::Unbind() const
{
    glBindVertexArray(0);
}

void VAO::Delete()
{
    if (m_ID != 0)
    {
        glDeleteVertexArrays(1, &m_ID);
        m_ID = 0;
    }
}

} // namespace AG