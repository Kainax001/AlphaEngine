#include "AlphaScene/MeshComponent.h"
#include "AlphaScene/Actor.h"

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

} // namespace AS
