#pragma once

#include "AlphaGraphic/core/RenderProxy.h"
#include "AlphaGraphic/buffer/SSBO.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace AG {

// Serializes a SceneProxy into global SSBOs every frame.
// Fixed binding points:  CameraSSBO=2, LightSSBO=3.
// Developers can register additional SSBOs via RegisterSSBO().
class BufferManager
{
public:
    static constexpr GLuint CAMERA_BINDING = 2;
    static constexpr GLuint LIGHT_BINDING  = 3;
    static constexpr int    MAX_LIGHTS     = 64;

    BufferManager();

    // Upload SceneProxy data to GPU SSBOs.
    void Update(const SceneProxy& proxy);

    // Bind all managed SSBOs to their binding points.
    void BindAll()   const;
    void UnbindAll() const;

    // Register a developer-created SSBO (raw pointer, not owned).
    void RegisterSSBO(const std::string& name, SSBO* ssbo);

    SSBO* GetCameraSSBO() const { return m_CameraSSBO.get(); }
    SSBO* GetLightSSBO()  const { return m_LightSSBO.get(); }

private:
    std::unique_ptr<SSBO> m_CameraSSBO;
    std::unique_ptr<SSBO> m_LightSSBO;
    std::unordered_map<std::string, SSBO*> m_CustomSSBOs;
};

} // namespace AG
