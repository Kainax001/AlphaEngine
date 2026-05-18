#include "AlphaGraphic/core/BufferManager.h"

#include <vector>

namespace AG {

BufferManager::BufferManager()
{
    m_CameraSSBO = std::make_unique<SSBO>(
        sizeof(CameraProxy), CAMERA_BINDING);

    m_LightSSBO = std::make_unique<SSBO>(
        sizeof(LightProxy) * MAX_LIGHTS, LIGHT_BINDING);
}

void BufferManager::Update(const SceneProxy& proxy)
{
    // Camera
    m_CameraSSBO->UpdateData(0, sizeof(CameraProxy), &proxy.camera);

    // Lights — resize if scene has more lights than initial allocation
    if (!proxy.lights.empty())
    {
        GLsizeiptr needed = static_cast<GLsizeiptr>(
            sizeof(LightProxy) * proxy.lights.size());
        m_LightSSBO->Resize(needed);
        m_LightSSBO->UpdateData(0, needed, proxy.lights.data());
    }

    // Custom SSBOs are updated by the developer directly
}

void BufferManager::BindAll() const
{
    m_CameraSSBO->Bind();
    m_LightSSBO->Bind();

    for (const auto& pair : m_CustomSSBOs)
        if (pair.second) pair.second->Bind();
}

void BufferManager::UnbindAll() const
{
    m_CameraSSBO->Unbind();
    m_LightSSBO->Unbind();
}

void BufferManager::RegisterSSBO(const std::string& name, SSBO* ssbo)
{
    m_CustomSSBOs[name] = ssbo;
}

} // namespace AG
