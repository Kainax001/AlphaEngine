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
    static constexpr GLuint CAMERA_BINDING   = 2;
    static constexpr GLuint LIGHT_BINDING    = 3;
    static constexpr GLuint TRIANGLE_BINDING = 4;
    static constexpr GLuint BVH_BINDING      = 5;
    static constexpr int    MAX_LIGHTS       = 64;

    BufferManager();

    // Upload full SceneProxy (camera + lights + triangles + BVH) to GPU SSBOs.
    void Update(const SceneProxy& proxy);

    // Upload only camera and lights — skip expensive geometry re-upload.
    // Call this every frame when geometry hasn't changed.
    void UpdateDynamic(const CameraProxy& camera,
                       const std::vector<LightProxy>& lights);

    // Bind all managed SSBOs to their binding points.
    void BindAll()   const;
    void UnbindAll() const;

    // Register a developer-created SSBO (raw pointer, not owned).
    void RegisterSSBO(const std::string& name, SSBO* ssbo);

    SSBO* GetCameraSSBO()   const { return m_CameraSSBO.get(); }
    SSBO* GetLightSSBO()    const { return m_LightSSBO.get(); }
    SSBO* GetTriangleSSBO() const { return m_TriangleSSBO.get(); }
    SSBO* GetBVHNodeSSBO()  const { return m_BVHNodeSSBO.get(); }

private:
    std::unique_ptr<SSBO> m_CameraSSBO;
    std::unique_ptr<SSBO> m_LightSSBO;
    std::unique_ptr<SSBO> m_TriangleSSBO;
    std::unique_ptr<SSBO> m_BVHNodeSSBO;
    std::unordered_map<std::string, SSBO*> m_CustomSSBOs;
};

} // namespace AG
