#pragma once

#include <glad/glad.h>

namespace AG {

class UBO
{
public:
    GLuint m_ID;

    UBO(GLsizeiptr size, GLuint bindingPoint);
    ~UBO();

    void Bind()   const;
    void Unbind() const;
    void Delete();

    void UpdateData  (GLintptr offset, GLsizeiptr size, const void* data);
    void LinkToShader(GLuint shaderID, const char* blockName, GLuint bindingPoint);
};

} // namespace AG