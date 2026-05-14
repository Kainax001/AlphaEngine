#pragma once
#include "Actor.h"
#include "RenderContext.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

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

    // Shared UBO management.
    // World owns the UBO; all MeshComponents receive a reference automatically in Render().
    AG::UBO* CreateSharedUBO(const std::string& blockName, GLsizeiptr size, GLuint binding);
    AG::UBO* GetSharedUBO   (const std::string& blockName) const;

private:
    void FlushPendingActors();

    struct SharedUBOEntry {
        std::unique_ptr<AG::UBO> ubo;
        GLuint                   binding = 0;
    };

    std::vector<std::shared_ptr<Actor>> m_Actors;
    std::vector<std::shared_ptr<Actor>> m_PendingAdd;
    std::vector<Actor*>                 m_PendingRemove;

    std::unordered_map<std::string, SharedUBOEntry> m_SharedUBOs;

    float m_TimeScale  = 1.0f;
    float m_FixedDT    = 1.0f / 60.0f;
    float m_FixedAccum = 0.0f;
};

} // namespace AS
