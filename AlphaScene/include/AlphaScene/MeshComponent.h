#pragma once
#include "Component.h"
#include "RenderContext.h"
#include <AlphaGraphic/AlphaGraphic.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace AS {

class MeshComponent : public Component {
public:
    explicit MeshComponent(Actor* owner);

    void LoadModel(const std::string& path);
    void SetShaderName(const std::string& name);
    void SetMaterial(std::shared_ptr<AG::Material> material);

    // Register an externally-owned UBO (e.g. World's SharedUBO).
    // MeshComponent holds a raw pointer only — no ownership.
    void RegisterUBO(const std::string& blockName, AG::UBO* ubo, GLuint binding);

    // Create and own a UBO for per-mesh data (material params, skin matrices, etc.).
    // Returns the raw pointer for immediate use; MeshComponent owns the lifetime.
    AG::UBO* CreateUBO(const std::string& blockName, GLsizeiptr size, GLuint binding);

    // Update data in an owned UBO created via CreateUBO().
    void UpdateUBO(const std::string& blockName, const void* data,
                   GLsizeiptr size, GLintptr offset = 0);

    void Render(const RenderContext& ctx);

    const std::shared_ptr<AG::Model>& GetModel() const { return m_Model; }

private:
    struct UBOSlot {
        AG::UBO*                 ubo     = nullptr; // always valid when slot exists
        GLuint                   binding = 0;
        std::unique_ptr<AG::UBO> owned;             // null = externally owned
        bool                     linked  = false;   // linked to current shader?
    };

    // When shader changes all slots need re-linking.
    void InvalidateLinks();

    std::shared_ptr<AG::Model>    m_Model;
    std::shared_ptr<AG::Material> m_Material;
    std::string                   m_ShaderName;
    std::string                   m_LastLinkedShader;

    std::unordered_map<std::string, UBOSlot> m_UBOSlots;
};

} // namespace AS
