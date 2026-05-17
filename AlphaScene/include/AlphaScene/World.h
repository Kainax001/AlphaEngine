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

    // Shared UBO — World owns, all MeshComponents receive reference automatically in Render().
    AG::UBO* CreateSharedUBO(const std::string& blockName, GLsizeiptr size, GLuint binding);
    AG::UBO* GetSharedUBO   (const std::string& blockName) const;

    // Activate the standard 3-light (Dir/Point/Spot) UBO system.
    // World::Render() will gather LightComponents and pack them into "LightBlock" each frame.
    void EnableDefaultLighting(GLuint binding = 1);

    // Serialization
    bool SaveScene (const std::string& path) const;
    bool LoadScene (const std::string& path, AC::Input* input = nullptr);

private:
    void FlushPendingActors();
    void UpdateDefaultLightingUBO(const RenderContext& ctx);

    // std140 layout matching Lit.frag LightBlock
    struct LightUBOData {
        glm::vec4 dirDirection;     // xyz = dir,   w = intensity
        glm::vec4 dirColor;         // xyz = color, w = 0
        glm::vec4 pointPosition;    // xyz = pos,   w = intensity
        glm::vec4 pointColor;       // xyz = color, w = constant
        glm::vec4 pointAttenuation; // x = linear,  y = quad, zw = 0
        glm::vec4 spotPosition;     // xyz = pos,   w = intensity
        glm::vec4 spotDirection;    // xyz = dir,   w = cutOff(cos)
        glm::vec4 spotColor;        // xyz = color, w = outerCutOff(cos)
        glm::vec4 spotAttenuation;  // x = linear,  y = quad, z = constant, w = 0
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
};

} // namespace AS
