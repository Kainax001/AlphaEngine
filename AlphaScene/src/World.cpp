#include "AlphaScene/World.h"
#include "AlphaScene/CameraComponent.h"
#include "AlphaScene/LightComponent.h"
#include "AlphaScene/MeshComponent.h"
#include "AlphaScene/InputComponent.h"
#include "AlphaScene/ComponentRegistry.h"

#include <AlphaUtil/Json.h>
#include <AlphaControler/AlphaControler.h>
#include <rapidjson/document.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>

namespace AS {

World::World()
{
    static bool registered = false;
    if (!registered)
    {
        ComponentRegistry::Register<CameraComponent>("CameraComponent");
        ComponentRegistry::Register<LightComponent> ("LightComponent");
        ComponentRegistry::Register<MeshComponent>  ("MeshComponent");
        ComponentRegistry::Register<InputComponent> ("InputComponent");
        registered = true;
    }
}
World::~World() { EndPlay(); }

// ---------------------------------------------------------------------------
// Actor management
// ---------------------------------------------------------------------------

Actor* World::SpawnActor(const std::string& name)
{
    auto  actor = std::make_shared<Actor>(name);
    Actor* ptr  = actor.get();
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
    if (it != m_SharedUBOs.end()) return it->second.ubo.get();

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

void World::EnableDefaultLighting(GLuint binding)
{
    m_LightBlockBinding      = binding;
    m_DefaultLightingEnabled = true;
    CreateSharedUBO("LightBlock", sizeof(LightUBOData), binding);
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
// Default lighting UBO update
// ---------------------------------------------------------------------------

void World::UpdateDefaultLightingUBO(const RenderContext& ctx)
{
    AG::UBO* ubo = GetSharedUBO("LightBlock");
    if (!ubo) return;

    LightUBOData data{};
    for (const LightData& ld : ctx.lights)
    {
        if (ld.type == LightType::Directional)
        {
            data.dirDirection = glm::vec4(ld.direction, ld.intensity);
            data.dirColor     = glm::vec4(ld.color,     0.0f);
        }
        else if (ld.type == LightType::Point)
        {
            data.pointPosition    = glm::vec4(ld.position,  ld.intensity);
            data.pointColor       = glm::vec4(ld.color,     ld.constant);
            data.pointAttenuation = glm::vec4(ld.linear, ld.quadratic, 0.0f, 0.0f);
        }
        else if (ld.type == LightType::Spot)
        {
            float cosInner = glm::cos(glm::radians(ld.cutOff));
            float cosOuter = glm::cos(glm::radians(ld.outerCutOff));
            data.spotPosition    = glm::vec4(ld.position,  ld.intensity);
            data.spotDirection   = glm::vec4(ld.direction, cosInner);
            data.spotColor       = glm::vec4(ld.color,     cosOuter);
            data.spotAttenuation = glm::vec4(ld.linear, ld.quadratic, ld.constant, 0.0f);
        }
    }
    ubo->UpdateData(0, sizeof(LightUBOData), &data);
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

    // Stage 2: Update default lighting UBO (if enabled)
    if (m_DefaultLightingEnabled)
        UpdateDefaultLightingUBO(ctx);

    // Stage 3: Draw meshes — auto-register all SharedUBOs
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
    m_DefaultLightingEnabled = false;
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------

bool World::SaveScene(const std::string& path) const
{
    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    doc.AddMember("version", 1, alloc);

    // defaultLighting
    rapidjson::Value lighting(rapidjson::kObjectType);
    lighting.AddMember("enabled", m_DefaultLightingEnabled, alloc);
    lighting.AddMember("binding", static_cast<int>(m_LightBlockBinding), alloc);
    doc.AddMember("defaultLighting", lighting, alloc);

    // actors (include pending — SaveScene may be called before BeginPlay flushes)
    rapidjson::Value actors(rapidjson::kArrayType);
    auto serializeList = [&](const std::vector<std::shared_ptr<Actor>>& list)
    {
        for (const auto& actor : list)
        {
            rapidjson::Value actorVal(rapidjson::kObjectType);
            actor->Serialize(actorVal, alloc);
            actors.PushBack(actorVal, alloc);
        }
    };
    serializeList(m_Actors);
    serializeList(m_PendingAdd);
    doc.AddMember("actors", actors, alloc);

    if (!AU::Json::SaveFile(path, doc))
    {
        std::cerr << "[World] SaveScene failed: " << path << "\n";
        return false;
    }

    std::cout << "[World] SaveScene: " << path << "\n";
    return true;
}

bool World::LoadScene(const std::string& path, AC::Input* input)
{
    rapidjson::Document doc;
    if (!AU::Json::LoadFile(path, doc))
    {
        std::cerr << "[World] LoadScene failed: " << path << "\n";
        return false;
    }

    // defaultLighting
    if (doc.HasMember("defaultLighting") && doc["defaultLighting"].IsObject())
    {
        const auto& dl = doc["defaultLighting"];
        bool enabled = false;
        GLuint binding = 1;
        if (dl.HasMember("enabled") && dl["enabled"].IsBool())  enabled = dl["enabled"].GetBool();
        if (dl.HasMember("binding") && dl["binding"].IsInt())   binding = static_cast<GLuint>(dl["binding"].GetInt());
        if (enabled) EnableDefaultLighting(binding);
    }

    // actors
    if (doc.HasMember("actors") && doc["actors"].IsArray())
    {
        for (const auto& actorVal : doc["actors"].GetArray())
        {
            if (!actorVal.HasMember("name") || !actorVal["name"].IsString()) continue;

            std::string name = actorVal["name"].GetString();
            auto* actor = SpawnActor(name);
            actor->Deserialize(actorVal, input);
        }
    }

    std::cout << "[World] LoadScene: " << path << "\n";
    return true;
}

} // namespace AS
