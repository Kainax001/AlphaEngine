#pragma once

#include <glad/glad.h>

namespace AG {

// Shader Storage Buffer Object — wraps GL_SHADER_STORAGE_BUFFER.
// Requires OpenGL 4.3+.
class SSBO
{
public:
    SSBO(GLsizeiptr size, GLuint binding, GLenum usage = GL_DYNAMIC_DRAW);
    ~SSBO();

    SSBO(const SSBO&)            = delete;
    SSBO& operator=(const SSBO&) = delete;

    void Bind()   const;
    void Unbind() const;

    void UpdateData(GLintptr offset, GLsizeiptr size, const void* data);

    // Reallocate if new data is larger than current allocation.
    void Resize(GLsizeiptr newSize);

    GLuint     GetID()      const { return m_ID; }
    GLuint     GetBinding() const { return m_Binding; }
    GLsizeiptr GetSize()    const { return m_Size; }

private:
    GLuint     m_ID      = 0;
    GLuint     m_Binding = 0;
    GLsizeiptr m_Size    = 0;
    GLenum     m_Usage   = GL_DYNAMIC_DRAW;
};

} // namespace AG
