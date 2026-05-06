#pragma once

#include <glad/glad.h>
#include "AlphaGraphic/buffer/VBO.h"

namespace AG {

class VAO
{
public:
    GLuint m_ID;

    VAO();
    ~VAO();

    void LinkAttrib(VBO& vbo, GLuint layout, GLuint numComponents,
                   GLenum type, GLsizeiptr stride, void* offset);

    void Bind()   const;
    void Unbind() const;
    void Delete();
};

} // namespace AG