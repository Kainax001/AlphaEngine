#include "AlphaGraphic/core/BufferManager.h"

#include <vector>

namespace AG {

BufferManager::BufferManager()
{
    m_CameraSSBO = std::make_unique<SSBO>(
        sizeof(CameraProxy), CAMERA_BINDING);

    // Start with 1-light size; Resize() grows to exact count each frame.
    // This ensures Lights.lights.length() in GLSL equals the actual light count.
    m_LightSSBO = std::make_unique<SSBO>(
        sizeof(LightProxy), LIGHT_BINDING);

    m_TriangleSSBO = std::make_unique<SSBO>(
        sizeof(TriangleProxy), TRIANGLE_BINDING);

    m_BVHNodeSSBO = std::make_unique<SSBO>(
        sizeof(BVHNodeProxy), BVH_BINDING);
}

void BufferManager::Update(const SceneProxy& proxy)
{
    // Camera
    m_CameraSSBO->UpdateData(0, sizeof(CameraProxy), &proxy.camera);

    // Lights — always resize to exact count so .length() is correct in GLSL
    if (!proxy.lights.empty())
    {
        GLsizeiptr needed = static_cast<GLsizeiptr>(
            sizeof(LightProxy) * proxy.lights.size());
        m_LightSSBO->Resize(needed);
        m_LightSSBO->UpdateData(0, needed, proxy.lights.data());
    }

    // Triangles (reordered by BVH build)
    if (!proxy.triangles.empty())
    {
        GLsizeiptr needed = static_cast<GLsizeiptr>(
            sizeof(TriangleProxy) * proxy.triangles.size());
        m_TriangleSSBO->Resize(needed);
        m_TriangleSSBO->UpdateData(0, needed, proxy.triangles.data());
    }

    // BVH nodes
    if (!proxy.bvhNodes.empty())
    {
        GLsizeiptr needed = static_cast<GLsizeiptr>(
            sizeof(BVHNodeProxy) * proxy.bvhNodes.size());
        m_BVHNodeSSBO->Resize(needed);
        m_BVHNodeSSBO->UpdateData(0, needed, proxy.bvhNodes.data());
    }

    // Custom SSBOs are updated by the developer directly
}

void BufferManager::UpdateDynamic(const CameraProxy& camera,
                                   const std::vector<LightProxy>& lights)
{
    m_CameraSSBO->UpdateData(0, sizeof(CameraProxy), &camera);

    if (!lights.empty())
    {
        GLsizeiptr needed = static_cast<GLsizeiptr>(sizeof(LightProxy) * lights.size());
        m_LightSSBO->Resize(needed);
        m_LightSSBO->UpdateData(0, needed, lights.data());
    }
}

void BufferManager::BindAll() const
{
    m_CameraSSBO->Bind();
    m_LightSSBO->Bind();
    m_TriangleSSBO->Bind();
    m_BVHNodeSSBO->Bind();

    for (const auto& pair : m_CustomSSBOs)
        if (pair.second) pair.second->Bind();
}

void BufferManager::UnbindAll() const
{
    m_CameraSSBO->Unbind();
    m_LightSSBO->Unbind();
    m_TriangleSSBO->Unbind();
    m_BVHNodeSSBO->Unbind();
}

void BufferManager::RegisterSSBO(const std::string& name, SSBO* ssbo)
{
    m_CustomSSBOs[name] = ssbo;
}

} // namespace AG
