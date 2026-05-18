#pragma once
#include "Actor.h"
#include "RenderContext.h"
#include <AlphaGraphic/AlphaGraphic.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace AC { class Input; }

namespace AS {

enum class RenderMode {
    Rasterization,  // default: existing 3-stage forward pipeline
    RayTracing,     // compute shader only, fullscreen blit
    Hybrid,         // rasterization + RT pass (future)
};

class World {
public:
    World();
    ~World();

    // Actor management
    Actor* SpawnActor(const std::string& name);
    void   DestroyActor(Actor* actor);
    Actor* FindActor(const std::string& name) const;

    // Lifecycle
    void BeginPlay();
    void Tick(float dt);
    void Render();
    void EndPlay();

    // Time control
    void  SetTimeScale(float s) { m_TimeScale = s; }
    float GetTimeScale()  const { return m_TimeScale; }
    float GetFixedDT()    const { return m_FixedDT; }
    void  SetFixedDT(float dt)  { m_FixedDT = dt; }

    // Shared UBO — World owns, all MeshComponents receive reference automatically.
    AG::UBO* CreateSharedUBO(const std::string& blockName, GLsizeiptr size, GLuint binding);
    AG::UBO* GetSharedUBO   (const std::string& blockName) const;

    // Activate the standard 3-light (Dir/Point/Spot) UBO system.
    void EnableDefaultLighting(GLuint binding = 1);

    // Ray Tracing
    // computePath: path to a .comp file
    // width/height: output image resolution
    void EnableRayTracing(const std::string& computePath,
                          int width, int height,
                          RenderMode mode = RenderMode::RayTracing);
    void DisableRayTracing();
    bool IsRayTracingEnabled() const { return m_RenderMode != RenderMode::Rasterization; }
    RenderMode GetRenderMode() const { return m_RenderMode; }

    // Register a developer-owned SSBO with the BufferManager.
    AG::SSBO* CreateRayTracingSSBO(const std::string& name,
                                   GLsizeiptr size, GLuint binding);

    // Serialization
    bool SaveScene (const std::string& path) const;
    bool LoadScene (const std::string& path, AC::Input* input = nullptr);

private:
    void FlushPendingActors();
    void UpdateDefaultLightingUBO(const RenderContext& ctx);
    AG::SceneProxy CollectSceneProxy() const;

    // std140 layout matching Lit.frag LightBlock
    struct LightUBOData {
        glm::vec4 dirDirection;
        glm::vec4 dirColor;
        glm::vec4 pointPosition;
        glm::vec4 pointColor;
        glm::vec4 pointAttenuation;
        glm::vec4 spotPosition;
        glm::vec4 spotDirection;
        glm::vec4 spotColor;
        glm::vec4 spotAttenuation;
    };

    struct SharedUBOEntry {
        std::unique_ptr<AG::UBO> ubo;
        GLuint                   binding = 0;
    };

    std::vector<std::shared_ptr<Actor>> m_Actors;
    std::vector<std::shared_ptr<Actor>> m_PendingAdd;
    std::vector<Actor*>                 m_PendingRemove;

    std::unordered_map<std::string, SharedUBOEntry> m_SharedUBOs;

    bool   m_DefaultLightingEnabled = false;
    GLuint m_LightBlockBinding      = 1;

    float m_TimeScale  = 1.0f;
    float m_FixedDT    = 1.0f / 60.0f;
    float m_FixedAccum = 0.0f;

    // Ray tracing state
    RenderMode m_RenderMode = RenderMode::Rasterization;

    std::unique_ptr<AG::ComputeShader>  m_ComputeShader;
    std::unique_ptr<AG::BufferManager>  m_BufferManager;
    std::unique_ptr<AG::FullscreenQuad> m_FullscreenQuad;

    GLuint m_RTOutputTexture = 0;
    int    m_RTWidth         = 0;
    int    m_RTHeight        = 0;

    // Developer-owned SSBOs registered via CreateRayTracingSSBO
    std::vector<std::unique_ptr<AG::SSBO>> m_OwnedRTSSBOs;
};

} // namespace AS
