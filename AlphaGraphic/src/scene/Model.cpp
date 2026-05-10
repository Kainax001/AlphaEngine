#include "AlphaGraphic/scene/Model.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>

namespace AG {

Model::Model(const std::string& path)
{
    LoadFromFile(path);
}

void Model::Draw() const
{
    if (m_Material)
    {
        // Material이 설정된 경우: Material 시스템만 사용한다.
        // m_MeshDiffuse(Assimp fallback)는 바인딩하지 않아 슬롯 충돌을 방지한다.
        m_Material->Bind();
        for (size_t i = 0; i < m_Meshes.size(); ++i)
            m_Meshes[i]->Draw();
        m_Material->Unbind();
    }
    else
    {
        // Material이 없는 경우: Assimp가 읽어온 메시별 diffuse 텍스처를 슬롯 0에 바인딩한다.
        for (size_t i = 0; i < m_Meshes.size(); ++i)
        {
            if (i < m_MeshDiffuse.size() && m_MeshDiffuse[i])
                m_MeshDiffuse[i]->Bind(0);
            m_Meshes[i]->Draw();
        }
    }
}

glm::vec3 Model::GetCenter() const
{
    return m_Transform.GetPosition();
}

void Model::LoadFromFile(const std::string& path)
{
    m_Path = path;

    size_t slash = path.find_last_of('/');
    if (slash == std::string::npos)
        slash = path.find_last_of('\\');
    m_Directory = (slash != std::string::npos) ? path.substr(0, slash) : ".";

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate       |
        aiProcess_FlipUVs           |
        aiProcess_CalcTangentSpace  |
        aiProcess_GenSmoothNormals
    );

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
    {
        std::cerr << "[Model] Assimp error: " << importer.GetErrorString() << "\n";
        return;
    }

    ProcessNode(scene->mRootNode, scene);
    std::cout << "[Model] Loaded " << m_Meshes.size() << " mesh(es) from: " << path << "\n";
}

void Model::ProcessNode(aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
        m_Meshes.push_back(ProcessMesh(scene->mMeshes[node->mMeshes[i]], scene));

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
        ProcessNode(node->mChildren[i], scene);
}

std::shared_ptr<StaticMesh> Model::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex v;
        v.Position  = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        v.Normal    = mesh->mNormals
                    ? glm::vec3(mesh->mNormals[i].x,    mesh->mNormals[i].y,    mesh->mNormals[i].z)
                    : glm::vec3(0.0f, 1.0f, 0.0f);
        v.TexCoords = mesh->mTextureCoords[0]
                    ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
                    : glm::vec2(0.0f, 0.0f);
        v.Tangent   = mesh->mTangents
                    ? glm::vec3(mesh->mTangents[i].x,   mesh->mTangents[i].y,   mesh->mTangents[i].z)
                    : glm::vec3(0.0f, 0.0f, 0.0f);
        v.Bitangent = mesh->mBitangents
                    ? glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z)
                    : glm::vec3(0.0f, 0.0f, 0.0f);
        vertices.push_back(v);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
            indices.push_back(face.mIndices[j]);
    }

    // MTL에서 diffuse 텍스처 로딩
    std::shared_ptr<Texture2D> diffuseTex = nullptr;
    if (mesh->mMaterialIndex < scene->mNumMaterials)
    {
        aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
        auto textures = LoadMaterialTextures(mat, aiTextureType_DIFFUSE, "diffuse");
        if (!textures.empty())
            diffuseTex = textures[0];
    }
    m_MeshDiffuse.push_back(diffuseTex);

    return std::make_shared<StaticMesh>(vertices, indices);
}

std::vector<std::shared_ptr<Texture2D>> Model::LoadMaterialTextures(
    aiMaterial* mat, aiTextureType type, const std::string& typeName)
{
    std::vector<std::shared_ptr<Texture2D>> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(type); ++i)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::string texPath = m_Directory + "/" + str.C_Str();

        auto it = m_TextureCache.find(texPath);
        if (it != m_TextureCache.end())
        {
            textures.push_back(it->second);
        }
        else
        {
            auto tex = std::make_shared<Texture2D>(texPath);
            m_TextureCache[texPath] = tex;
            textures.push_back(tex);
        }
    }

    return textures;
}

} // namespace AG
