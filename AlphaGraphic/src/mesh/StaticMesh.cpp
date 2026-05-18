#include "AlphaGraphic/mesh/StaticMesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>

namespace AG {

StaticMesh::StaticMesh(const std::string& path)
{
    LoadFromFile(path);
}

StaticMesh::StaticMesh(std::vector<Vertex>& vertices, std::vector<GLuint>& indices)
{
    Setup(vertices, indices);
}

void StaticMesh::Draw() const
{
    m_VAO->Bind();
    glDrawElements(GL_TRIANGLES, (int)m_IndexCount, GL_UNSIGNED_INT, 0);
    m_VAO->Unbind();
}

void StaticMesh::Bind() const
{
    m_VAO->Bind();
}

void StaticMesh::Unbind() const
{
    m_VAO->Unbind();
}

unsigned int StaticMesh::GetVAO() const
{
    return m_VAO->m_ID;
}

void StaticMesh::Setup(std::vector<Vertex>& vertices, std::vector<GLuint>& indices)
{
    m_Vertices    = vertices;
    m_Indices     = indices;
    m_VertexCount = (unsigned int)vertices.size();
    m_IndexCount  = (unsigned int)indices.size();

    m_VAO = std::make_unique<VAO>();
    m_VBO = std::make_unique<VBO>(vertices);
    m_EBO = std::make_unique<EBO>(indices);

    m_VAO->Bind();
    m_EBO->Bind();

    m_VAO->LinkAttrib(*m_VBO, 0, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, Position));
    m_VAO->LinkAttrib(*m_VBO, 1, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    m_VAO->LinkAttrib(*m_VBO, 2, 2, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    m_VAO->LinkAttrib(*m_VBO, 3, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
    m_VAO->LinkAttrib(*m_VBO, 4, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

    m_VAO->Unbind();
    m_VBO->Unbind();
    m_EBO->Unbind();
}

void StaticMesh::LoadFromFile(const std::string& path)
{
    m_Path = path;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate      |
        aiProcess_FlipUVs          |
        aiProcess_CalcTangentSpace |
        aiProcess_GenNormals
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "[AlphaGraphic] ERROR::STATIC_MESH::Assimp: " << importer.GetErrorString() << "\n";
        return;
    }

    if (scene->mNumMeshes == 0)
    {
        std::cerr << "[AlphaGraphic] ERROR::STATIC_MESH::No meshes found in: " << path << "\n";
        return;
    }

    aiMesh* mesh = scene->mMeshes[0];

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        vertex.Position  = { mesh->mVertices[i].x,  mesh->mVertices[i].y,  mesh->mVertices[i].z  };
        vertex.Normal    = { mesh->mNormals[i].x,   mesh->mNormals[i].y,   mesh->mNormals[i].z   };
        vertex.TexCoords = mesh->mTextureCoords[0]
                         ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
                         : glm::vec2(0.0f);
        vertex.Tangent   = { mesh->mTangents[i].x,   mesh->mTangents[i].y,   mesh->mTangents[i].z   };
        vertex.Bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    Setup(vertices, indices);
}

} // namespace AG