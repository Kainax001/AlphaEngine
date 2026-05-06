#pragma once

#include <glad/glad.h>

namespace AG {

class FBO
{
public:
    GLuint m_ID;

    FBO();
    ~FBO();

    void Bind()   const;
    void Unbind() const;
    void Delete();
};

} // namespace AG
