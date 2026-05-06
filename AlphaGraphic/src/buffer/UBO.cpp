#include "AlphaGraphic/buffer/UBO.h"

#include <iostream>

namespace AG {

UBO::UBO(GLsizeiptr size, GLuint bindingPoint)
{
    glGenBuffers(1, &m_ID);
    glBindBuffer(GL_UNIFORM_BUFFER, m_ID);
    glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, m_ID);
}

UBO::~UBO()
{
    Delete();
}

void UBO::Bind() const
{
    glBindBuffer(GL_UNIFORM_BUFFER, m_ID);
}

void UBO::Unbind() const
{
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UBO::Delete()
{
    if (m_ID != 0)
    {
        glDeleteBuffers(1, &m_ID);
        m_ID = 0;
    }
}

void UBO::UpdateData(GLintptr offset, GLsizeiptr size, const void* data)
{
    glBindBuffer(GL_UNIFORM_BUFFER, m_ID);
    glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UBO::LinkToShader(GLuint shaderID, const char* blockName, GLuint bindingPoint)
{
    GLuint blockIndex = glGetUniformBlockIndex(shaderID, blockName);

    if (blockIndex != GL_INVALID_INDEX)
    {
        glUniformBlockBinding(shaderID, blockIndex, bindingPoint);
    }
    else
    {
        std::cout << "[AlphaGraphic] WARNING::UBO: Uniform block '"
                  << blockName << "' not found in shader!\n";
    }
}

} // namespace AG