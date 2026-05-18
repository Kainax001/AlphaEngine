#include "AlphaScene/MeshComponent.h"
#include "AlphaScene/Actor.h"
#include <AlphaGraphic/mesh/StaticMesh.h>
#include <glm/gtc/matrix_inverse.hpp>

namespace AS {

MeshComponent::MeshComponent(Actor* owner)
    : Component(owner)
{
}

void MeshComponent::LoadModel(const std::string& path)
{
    m_Model = std::make_shared<AG::Model>(path);
}

void MeshComponent::SetShaderName(const std::string& name)
{
    if (m_ShaderName == name) return;
    m_ShaderName = name;
    InvalidateLinks();
}

void MeshComponent::SetMaterial(std::shared_ptr<AG::Material> material)
{
    m_Material = std::move(material);
}

// ---------------------------------------------------------------------------
// UBO management
// ---------------------------------------------------------------------------

void MeshComponent::RegisterUBO(const std::string& blockName, AG::UBO* ubo, GLuint binding)
{
    auto it = m_UBOSlots.find(blockName);
    if (it != m_UBOSlots.end())
    {
        // Update pointer/binding; re-link on next Render
        it->second.ubo     = ubo;
        it->second.binding = binding;
        it->second.linked  = false;
        return;
    }

    UBOSlot slot;
    slot.ubo     = ubo;
    slot.binding = binding;
    m_UBOSlots.emplace(blockName, std::move(slot));
}

AG::UBO* MeshComponent::CreateUBO(const std::string& blockName,
                                   GLsizeiptr size, GLuint binding)
{
    auto it = m_UBOSlots.find(blockName);
    if (it != m_UBOSlots.end())
        return it->second.ubo; // already exists

    UBOSlot slot;
    slot.owned   = std::make_unique<AG::UBO>(size, binding);
    slot.ubo     = slot.owned.get();
    slot.binding = binding;
    AG::UBO* ptr = slot.ubo;
    m_UBOSlots.emplace(blockName, std::move(slot));
    return ptr;
}

void MeshComponent::UpdateUBO(const std::string& blockName, const void* data,
                               GLsizeiptr size, GLintptr offset)
{
    auto it = m_UBOSlots.find(blockName);
    if (it != m_UBOSlots.end() && it->second.ubo)
        it->second.ubo->UpdateData(offset, size, data);
}

void MeshComponent::InvalidateLinks()
{
    for (auto& [name, slot] : m_UBOSlots)
        slot.linked = false;
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void MeshComponent::Render(const RenderContext& ctx)
{
    if (!m_Model || m_ShaderName.empty()) return;

    AG::Shader& shader = AG::ShaderLibrary::Get(m_ShaderName);

    // Re-link all UBO slots when shader changes
    if (m_LastLinkedShader != m_ShaderName)
    {
        InvalidateLinks();
        m_LastLinkedShader = m_ShaderName;
    }

    for (auto& [blockName, slot] : m_UBOSlots)
    {
        if (!slot.linked && slot.ubo)
        {
            slot.ubo->LinkToShader(shader.m_ID, blockName.c_str(), slot.binding);
            slot.linked = true;
        }
    }

    shader.Use();
    shader.SetMat4("u_View",       ctx.view);
    shader.SetMat4("u_Projection", ctx.projection);
    shader.SetVec3("u_ViewPos",    ctx.viewPos);
    shader.SetMat4("u_Model",      m_Owner->GetTransform().GetModelMatrix());
    shader.SetInt ("u_Diffuse",    0);

    m_Model->Draw();
}

void MeshComponent::Serialize(rapidjson::Value& out,
                               rapidjson::Document::AllocatorType& alloc) const
{
    rapidjson::Value modelPath(rapidjson::kStringType);
    if (m_Model)
    {
        const std::string& path = m_Model->GetPath();
        modelPath.SetString(path.c_str(), static_cast<rapidjson::SizeType>(path.size()), alloc);
    }
    else
    {
        modelPath.SetString("", 0, alloc);
    }

    rapidjson::Value shaderName(rapidjson::kStringType);
    shaderName.SetString(m_ShaderName.c_str(),
                         static_cast<rapidjson::SizeType>(m_ShaderName.size()), alloc);

    out.AddMember("modelPath",  modelPath,  alloc);
    out.AddMember("shaderName", shaderName, alloc);
}

void MeshComponent::Deserialize(const rapidjson::Value& in)
{
    if (in.HasMember("modelPath") && in["modelPath"].IsString())
    {
        std::string path = in["modelPath"].GetString();
        if (!path.empty()) LoadModel(path);
    }
    if (in.HasMember("shaderName") && in["shaderName"].IsString())
        SetShaderName(in["shaderName"].GetString());
}

void MeshComponent::FillTriangles(std::vector<AG::TriangleProxy>& out, int matBase) const
{
    if (!m_Model) return;

    glm::mat4 model     = m_Owner->GetTransform().GetModelMatrix();
    glm::mat3 normalMat = glm::mat3(glm::inverseTranspose(glm::mat3(model)));

    const auto& meshes = m_Model->GetMeshes();
    for (size_t meshIdx = 0; meshIdx < meshes.size(); meshIdx++)
    {
        const auto&                    mesh    = meshes[meshIdx];
        const std::vector<AG::Vertex>& verts   = mesh->GetVertices();
        const std::vector<GLuint>&     indices = mesh->GetIndices();
        float matIdxF = static_cast<float>(matBase + static_cast<int>(meshIdx));

        for (size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            const AG::Vertex& a = verts[indices[i]];
            const AG::Vertex& b = verts[indices[i + 1]];
            const AG::Vertex& c = verts[indices[i + 2]];

            AG::TriangleProxy tri;

            glm::vec4 wa = model * glm::vec4(a.Position.x, a.Position.y, a.Position.z, 1.0f);
            glm::vec4 wb = model * glm::vec4(b.Position.x, b.Position.y, b.Position.z, 1.0f);
            glm::vec4 wc = model * glm::vec4(c.Position.x, c.Position.y, c.Position.z, 1.0f);
            tri.v0 = glm::vec4(wa.x, wa.y, wa.z, 0.0f);
            tri.v1 = glm::vec4(wb.x, wb.y, wb.z, 0.0f);
            tri.v2 = glm::vec4(wc.x, wc.y, wc.z, 0.0f);

            glm::vec3 na = glm::normalize(normalMat * a.Normal);
            glm::vec3 nb = glm::normalize(normalMat * b.Normal);
            glm::vec3 nc = glm::normalize(normalMat * c.Normal);
            tri.n0 = glm::vec4(na.x, na.y, na.z, 0.0f);
            tri.n1 = glm::vec4(nb.x, nb.y, nb.z, 0.0f);
            tri.n2 = glm::vec4(nc.x, nc.y, nc.z, 0.0f);

            tri.uv01   = glm::vec4(a.TexCoords.x, a.TexCoords.y,
                                   b.TexCoords.x, b.TexCoords.y);
            // w is filled later by World with the deduplicated specular slot index
            tri.uv2mat = glm::vec4(c.TexCoords.x, c.TexCoords.y, matIdxF, matIdxF);

            out.push_back(tri);
        }
    }
}

std::vector<GLuint> MeshComponent::GetDiffuseTextureIDs() const
{
    std::vector<GLuint> ids;
    if (!m_Model) return ids;
    for (const auto& tex : m_Model->GetMeshDiffuse())
        ids.push_back(tex ? tex->GetID() : 0u);
    return ids;
}

std::vector<GLuint> MeshComponent::GetSpecularTextureIDs() const
{
    std::vector<GLuint> ids;
    if (!m_Model) return ids;
    for (const auto& tex : m_Model->GetMeshSpecular())
        ids.push_back(tex ? tex->GetID() : 0u);
    return ids;
}

AG::MeshProxy MeshComponent::ToProxy() const
{
    AG::MeshProxy proxy{};
    glm::mat4 model = m_Owner->GetTransform().GetModelMatrix();
    proxy.modelMatrix  = model;
    proxy.normalMatrix = glm::mat4(glm::inverseTranspose(glm::mat3(model)));

    // Approximate AABB as a sphere centered on the actor's world position.
    glm::vec3 pos = m_Owner->GetTransform().GetPosition();
    proxy.boundsCenter = glm::vec4(pos, 1.0f); // w = radius placeholder

    return proxy;
}

} // namespace AS
