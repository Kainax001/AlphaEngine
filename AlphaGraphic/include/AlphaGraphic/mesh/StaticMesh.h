#pragma once

#include "AlphaGraphic/mesh/Mesh.h"
#include "AlphaGraphic/buffer/VAO.h"
#include "AlphaGraphic/buffer/VBO.h"
#include "AlphaGraphic/buffer/EBO.h"

#include <string>
#include <memory>

namespace AG {

class StaticMesh : public Mesh
{
public:
    StaticMesh(const std::string& path);
    StaticMesh(std::vector<Vertex>& vertices, std::vector<GLuint>& indices);
    ~StaticMesh() = default;

    void Draw()   const override;
    void Bind()   const override;
    void Unbind() const override;

    unsigned int GetVAO() const override;

    const std::string& GetPath() const { return m_Path; }

    // CPU-side geometry access (retained after GPU upload for ray tracing)
    const std::vector<Vertex>& GetVertices() const { return m_Vertices; }
    const std::vector<GLuint>& GetIndices()  const { return m_Indices; }

private:
    void Setup(std::vector<Vertex>& vertices, std::vector<GLuint>& indices);
    void LoadFromFile(const std::string& path);

    std::unique_ptr<VAO>  m_VAO;
    std::unique_ptr<VBO>  m_VBO;
    std::unique_ptr<EBO>  m_EBO;
    std::string           m_Path;

    std::vector<Vertex>   m_Vertices;
    std::vector<GLuint>   m_Indices;
};

} // namespace AG