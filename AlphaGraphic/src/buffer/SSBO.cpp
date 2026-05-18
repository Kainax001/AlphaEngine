#include "AlphaGraphic/buffer/SSBO.h"

#include <iostream>

namespace AG {

SSBO::SSBO(GLsizeiptr size, GLuint binding, GLenum usage)
    : m_Binding(binding), m_Size(size), m_Usage(usage)
{
    glGenBuffers(1, &m_ID);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ID);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, usage);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_ID);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

SSBO::~SSBO()
{
    if (m_ID != 0)
        glDeleteBuffers(1, &m_ID);
}

void SSBO::Bind() const
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ID);
}

void SSBO::Unbind() const
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void SSBO::UpdateData(GLintptr offset, GLsizeiptr size, const void* data)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ID);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void SSBO::Resize(GLsizeiptr newSize)
{
    if (newSize == m_Size) return;
    m_Size = newSize;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ID);
    glBufferData(GL_SHADER_STORAGE_BUFFER, newSize, nullptr, m_Usage);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_Binding, m_ID);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

} // namespace AG
