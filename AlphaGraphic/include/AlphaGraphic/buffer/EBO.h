#pragma once

#include <glad/glad.h>
#include <vector>

namespace AG {

class EBO
{
public:
    GLuint m_ID;

    EBO(std::vector<GLuint>& indices);
    ~EBO();

    void Bind()   const;
    void Unbind() const;
    void Delete();
};

} // namespace AG