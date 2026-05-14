#include "AlphaScene/World.h"
#include "AlphaScene/CameraComponent.h"
#include "AlphaScene/LightComponent.h"
#include "AlphaScene/MeshComponent.h"
#include <algorithm>
#include <iostream>

namespace AS {

World::World()  = default;
World::~World() { EndPlay(); }

// ---------------------------------------------------------------------------
// Actor management
// ---------------------------------------------------------------------------

Actor* World::SpawnActor(const std::string& name)
{
    auto actor = std::make_shared<Actor>(name);
    Actor* ptr = actor.get();
    m_PendingAdd.push_back(std::move(actor));
    return ptr;
}

void World::DestroyActor(Actor* actor)
{
    m_PendingRemove.push_back(actor);
}

Actor* World::FindActor(const std::string& name) const
{
    for (const auto& a : m_Actors)
        if (a->GetName() == name) return a.get();
    return nullptr;
}

// ---------------------------------------------------------------------------
// Shared UBO
// ---------------------------------------------------------------------------

AG::UBO* World::CreateSharedUBO(const std::string& blockName,
                                  GLsizeiptr size, GLuint binding)
{
    auto it = m_SharedUBOs.find(blockName);
    if (it != m_SharedUBOs.end())
        return it->second.ubo.get();

    SharedUBOEntry entry;
    entry.ubo     = std::make_unique<AG::UBO>(size, binding);
    entry.binding = binding;
    AG::UBO* ptr  = entry.ubo.get();
    m_SharedUBOs.emplace(blockName, std::move(entry));
    return ptr;
}

AG::UBO* World::GetSharedUBO(const std::string& blockName) const
{
    auto it = m_SharedUBOs.find(blockName);
    return (it != m_SharedUBOs.end()) ? it->second.ubo.get() : nullptr;
}

// ---------------------------------------------------------------------------
// Pending flush
// ---------------------------------------------------------------------------

void World::FlushPendingActors()
{
    for (auto& actor : m_PendingAdd)
    {
        actor->BeginPlay();
        m_Actors.push_back(std::move(actor));
    }
    m_PendingAdd.clear();

    for (Actor* target : m_PendingRemove)
    {
        auto it = std::find_if(m_Actors.begin(), m_Actors.end(),
            [target](const std::shared_ptr<Actor>& a){ return a.get() == target; });
        if (it != m_Actors.end())
        {
            (*it)->EndPlay();
            m_Actors.erase(it);
        }
    }
    m_PendingRemove.clear();
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void World::BeginPlay()
{
    FlushPendingActors();
}

void World::Tick(float dt)
{
    FlushPendingActors();

    float scaledDt = dt * m_TimeScale;

    m_FixedAccum += scaledDt;
    while (m_FixedAccum >= m_FixedDT)
    {
        for (auto& actor : m_Actors)
            actor->FixedTick(m_FixedDT);
        m_FixedAccum -= m_FixedDT;
    }

    for (auto& actor : m_Actors)
        actor->Tick(scaledDt);
}

void World::Render()
{
    RenderContext ctx;

    // Stage 1: Gather camera + lights
    for (auto& actor : m_Actors)
    {
        if (!actor->IsActive()) continue;

        if (auto* cam = actor->GetComponent<CameraComponent>())
        {
            ctx.view       = cam->GetViewMatrix();
            ctx.projection = cam->GetProjectionMatrix(cam->GetAspect());
            ctx.viewPos    = actor->GetTransform().GetPosition();
        }

        if (auto* light = actor->GetComponent<LightComponent>())
            ctx.lights.push_back(light->GetLightData());
    }

    // Stage 2: Draw meshes.
    // Auto-register all SharedUBOs on each MeshComponent so they don't need
    // to know which UBOs exist — they just get linked on first Render().
    for (auto& actor : m_Actors)
    {
        if (!actor->IsActive()) continue;
        if (auto* mesh = actor->GetComponent<MeshComponent>())
        {
            for (const auto& [blockName, entry] : m_SharedUBOs)
                mesh->RegisterUBO(blockName, entry.ubo.get(), entry.binding);

            mesh->Render(ctx);
        }
    }
}

void World::EndPlay()
{
    for (auto& actor : m_Actors)
        actor->EndPlay();
    m_Actors.clear();
    m_PendingAdd.clear();
    m_PendingRemove.clear();
    m_SharedUBOs.clear();
}

} // namespace AS
