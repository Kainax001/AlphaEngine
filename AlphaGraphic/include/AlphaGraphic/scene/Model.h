#pragma once

#include "AlphaGraphic/mesh/StaticMesh.h"
#include "AlphaGraphic/scene/Material.h"
#include "AlphaGraphic/scene/Transform.h"
#include "AlphaGraphic/texture/Texture2D.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

namespace AG {

class Model
{
public:
    // ===== 기본 생성자 =====
    // 모델을 나중에 로드할 때 사용
    Model() = default;

    // ===== 매개변수 생성자 =====
    // 파일 경로를 받아서 모델 로드
    Model(const std::string& path);

    void Draw() const;

    glm::vec3 GetCenter() const;

    Transform&       GetTransform()       { return m_Transform; }
    const Transform& GetTransform() const { return m_Transform; }

    void SetMaterial(std::shared_ptr<Material> material) { m_Material = material; }

private:
    void LoadFromFile(const std::string& path);
    void ProcessNode(aiNode* node, const aiScene* scene);
    std::shared_ptr<StaticMesh> ProcessMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<std::shared_ptr<Texture2D>> LoadMaterialTextures(aiMaterial* mat,
                                                                  aiTextureType type,
                                                                  const std::string& typeName);

    std::vector<std::shared_ptr<StaticMesh>>              m_Meshes;
    std::vector<std::shared_ptr<Texture2D>>               m_MeshDiffuse; // 메시별 diffuse (null 가능)
    std::shared_ptr<Material>                             m_Material;
    Transform                                             m_Transform;
    std::string                                           m_Path;
    std::string                                           m_Directory;

    std::unordered_map<std::string, std::shared_ptr<Texture2D>> m_TextureCache;
};

} // namespace AG