#pragma once

#include <glad/glad.h>
#include <vector>

#include "AlphaGraphic/mesh/Mesh.h"

namespace AG {

class VBO
{
public:
    GLuint m_ID;

    VBO(std::vector<Vertex>& vertices);
    ~VBO();

    void Bind()   const;
    void Unbind() const;
    void Delete();
};

} // namespace AG