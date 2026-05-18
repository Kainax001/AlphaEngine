#include "AlphaScene/World.h"
#include <AlphaGraphic/core/BVHBuilder.h>
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
#include <unordered_map>
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
World::~World()
{
    // Release RT texture before EndPlay clears actors.
    if (m_RTOutputTexture != 0)
    {
        glDeleteTextures(1, &m_RTOutputTexture);
        m_RTOutputTexture = 0;
    }
    EndPlay();
}

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
    // Mark RT geometry dirty only when the actor list actually changes
    if (!m_PendingAdd.empty() || !m_PendingRemove.empty())
        m_RTGeoDirty = true;

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

AG::SceneProxy World::CollectSceneProxy() const
{
    AG::SceneProxy proxy;
    int matBase = 0;

    for (const auto& actor : m_Actors)
    {
        if (!actor->IsActive()) continue;
        if (auto* cam = actor->GetComponent<CameraComponent>())
            proxy.camera = cam->ToProxy();
        if (auto* light = actor->GetComponent<LightComponent>())
            proxy.lights.push_back(light->ToProxy());
        if (auto* mesh = actor->GetComponent<MeshComponent>())
        {
            proxy.meshes.push_back(mesh->ToProxy());
            mesh->FillTriangles(proxy.triangles, matBase);

            // Collect diffuse + specular texture IDs in material-index order
            for (GLuint id : mesh->GetDiffuseTextureIDs())
                proxy.diffuseTexIDs.push_back(id);
            for (GLuint id : mesh->GetSpecularTextureIDs())
                proxy.specularTexIDs.push_back(id);

            if (mesh->GetModel())
                matBase += static_cast<int>(mesh->GetModel()->GetMeshes().size());
        }
    }
    return proxy;
}

void World::EnableRayTracing(const std::string& computePath,
                              int width, int height, RenderMode mode)
{
    m_RenderMode = mode;
    m_RTWidth    = width;
    m_RTHeight   = height;

    m_ComputeShader  = std::make_unique<AG::ComputeShader>(computePath);
    m_BufferManager  = std::make_unique<AG::BufferManager>();
    m_FullscreenQuad = std::make_unique<AG::FullscreenQuad>();

    // Create GL_RGBA32F output texture
    if (m_RTOutputTexture != 0)
        glDeleteTextures(1, &m_RTOutputTexture);

    glGenTextures(1, &m_RTOutputTexture);
    glBindTexture(GL_TEXTURE_2D, m_RTOutputTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height,
                 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "[World] Ray tracing enabled: " << computePath
              << " (" << width << "x" << height << ")\n";
}

void World::DisableRayTracing()
{
    m_RenderMode = RenderMode::Rasterization;
    m_ComputeShader.reset();
    m_BufferManager.reset();
    m_FullscreenQuad.reset();

    if (m_RTOutputTexture != 0)
    {
        glDeleteTextures(1, &m_RTOutputTexture);
        m_RTOutputTexture = 0;
    }
}

AG::SSBO* World::CreateRayTracingSSBO(const std::string& name,
                                       GLsizeiptr size, GLuint binding)
{
    auto ssbo = std::make_unique<AG::SSBO>(size, binding);
    AG::SSBO* ptr = ssbo.get();
    if (m_BufferManager) m_BufferManager->RegisterSSBO(name, ptr);
    m_OwnedRTSSBOs.push_back(std::move(ssbo));
    return ptr;
}

void World::Render()
{
    RenderContext ctx;

    // Stage 1: Gather camera + lights (also collect proxy for RT)
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

    // Stage 2a: Update default lighting UBO (rasterization)
    if (m_DefaultLightingEnabled)
        UpdateDefaultLightingUBO(ctx);

    // Stage 3a: Rasterization pass
    if (m_RenderMode == RenderMode::Rasterization ||
        m_RenderMode == RenderMode::Hybrid)
    {
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

    // Stage 3b: Ray tracing pass
    if ((m_RenderMode == RenderMode::RayTracing ||
         m_RenderMode == RenderMode::Hybrid)
        && m_ComputeShader && m_ComputeShader->IsValid()
        && m_RTOutputTexture != 0 && m_BufferManager)
    {
        if (m_RTGeoDirty || m_CachedTriangles.empty())
        {
            // Full scene collection — only runs when geometry changes
            AG::SceneProxy proxy = CollectSceneProxy();

            // Deduplicate diffuse + specular textures independently.
            // Remap triangle.uv2mat.z → unique diffuse slot
            //              triangle.uv2mat.w → unique specular slot
            {
                auto buildDedup = [](const std::vector<GLuint>& ids,
                                     std::unordered_map<GLuint,int>& slotMap,
                                     std::vector<GLuint>& unique)
                {
                    for (GLuint id : ids)
                        if (slotMap.find(id) == slotMap.end())
                        {
                            slotMap[id] = static_cast<int>(unique.size());
                            unique.push_back(id);
                        }
                };

                // Diffuse: include texID=0 (shows as black → handled by shader fallback)
                std::unordered_map<GLuint,int> diffSlot;
                std::vector<GLuint> uniqueDiff;
                buildDedup(proxy.diffuseTexIDs, diffSlot, uniqueDiff);

                // Specular: exclude texID=0 — those meshes get sentinel -1
                // so the shader applies a surface-type default instead of sampling zero
                std::unordered_map<GLuint,int> specSlot;
                std::vector<GLuint> uniqueSpec;
                for (GLuint id : proxy.specularTexIDs)
                    if (id != 0 && specSlot.find(id) == specSlot.end())
                    {
                        specSlot[id] = static_cast<int>(uniqueSpec.size());
                        uniqueSpec.push_back(id);
                    }

                for (auto& tri : proxy.triangles)
                {
                    int origIdx = static_cast<int>(tri.uv2mat.z);
                    int nDiff   = static_cast<int>(proxy.diffuseTexIDs.size());
                    int nSpec   = static_cast<int>(proxy.specularTexIDs.size());

                    if (origIdx >= 0 && origIdx < nDiff)
                        tri.uv2mat.z = static_cast<float>(
                            diffSlot.at(proxy.diffuseTexIDs[origIdx]));

                    if (origIdx >= 0 && origIdx < nSpec)
                    {
                        GLuint sid = proxy.specularTexIDs[origIdx];
                        // -1.0 = no spec map → shader uses per-albedo default
                        tri.uv2mat.w = (sid == 0)
                            ? -1.0f
                            : static_cast<float>(specSlot.at(sid));
                    }
                    else
                    {
                        tri.uv2mat.w = -1.0f;
                    }
                }
                proxy.diffuseTexIDs  = std::move(uniqueDiff);
                proxy.specularTexIDs = std::move(uniqueSpec);
            }

            if (!proxy.triangles.empty())
                proxy.bvhNodes = AG::BVHBuilder::Build(proxy.triangles);

            // Upload everything to GPU before moving into cache
            m_BufferManager->Update(proxy);

            m_CachedTriangles      = std::move(proxy.triangles);
            m_CachedBVHNodes       = std::move(proxy.bvhNodes);
            m_CachedDiffuseTexIDs  = std::move(proxy.diffuseTexIDs);
            m_CachedSpecularTexIDs = std::move(proxy.specularTexIDs);
            m_RTGeoDirty = false;

            std::cout << "[World] BVH rebuilt: "
                      << m_CachedTriangles.size()      << " tris, "
                      << m_CachedBVHNodes.size()       << " nodes, "
                      << m_CachedDiffuseTexIDs.size()  << " diff / "
                      << m_CachedSpecularTexIDs.size() << " spec textures\n";
        }
        else
        {
            // Fast path: only camera + lights — no FillTriangles, no BVH, no SSBO copy
            AG::CameraProxy camProxy{};
            std::vector<AG::LightProxy> lightProxies;
            for (const auto& actor : m_Actors)
            {
                if (!actor->IsActive()) continue;
                if (auto* cam = actor->GetComponent<CameraComponent>())
                    camProxy = cam->ToProxy();
                if (auto* light = actor->GetComponent<LightComponent>())
                    lightProxies.push_back(light->ToProxy());
            }
            m_BufferManager->UpdateDynamic(camProxy, lightProxies);
        }

        m_BufferManager->BindAll();

        // Diffuse textures → units 5-12
        static constexpr int MAX_TEX = 8;
        int numDiff = std::min(static_cast<int>(m_CachedDiffuseTexIDs.size()), MAX_TEX);
        for (int i = 0; i < numDiff; i++)
        {
            glActiveTexture(GL_TEXTURE5 + i);
            glBindTexture(GL_TEXTURE_2D, m_CachedDiffuseTexIDs[i]);
        }
        // Specular textures → units 13-20
        int numSpec = std::min(static_cast<int>(m_CachedSpecularTexIDs.size()), MAX_TEX);
        for (int i = 0; i < numSpec; i++)
        {
            glActiveTexture(GL_TEXTURE13 + i);
            glBindTexture(GL_TEXTURE_2D, m_CachedSpecularTexIDs[i]);
        }

        glBindImageTexture(0, m_RTOutputTexture, 0, GL_FALSE, 0,
                           GL_WRITE_ONLY, GL_RGBA32F);

        m_ComputeShader->Use();
        // Diffuse samplers → texture units 5-12
        m_ComputeShader->SetInt("u_Tex0", 5);
        m_ComputeShader->SetInt("u_Tex1", 6);
        m_ComputeShader->SetInt("u_Tex2", 7);
        m_ComputeShader->SetInt("u_Tex3", 8);
        m_ComputeShader->SetInt("u_Tex4", 9);
        m_ComputeShader->SetInt("u_Tex5", 10);
        m_ComputeShader->SetInt("u_Tex6", 11);
        m_ComputeShader->SetInt("u_Tex7", 12);
        // Specular samplers → texture units 13-20
        m_ComputeShader->SetInt("u_SpecTex0", 13);
        m_ComputeShader->SetInt("u_SpecTex1", 14);
        m_ComputeShader->SetInt("u_SpecTex2", 15);
        m_ComputeShader->SetInt("u_SpecTex3", 16);
        m_ComputeShader->SetInt("u_SpecTex4", 17);
        m_ComputeShader->SetInt("u_SpecTex5", 18);
        m_ComputeShader->SetInt("u_SpecTex6", 19);
        m_ComputeShader->SetInt("u_SpecTex7", 20);
        m_ComputeShader->Dispatch(
            static_cast<GLuint>((m_RTWidth  + 15) / 16),
            static_cast<GLuint>((m_RTHeight + 15) / 16));
        m_ComputeShader->MemoryBarrier();

        m_BufferManager->UnbindAll();

        if (m_FullscreenQuad)
            m_FullscreenQuad->Draw(m_RTOutputTexture);
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
    m_OwnedRTSSBOs.clear();
    m_CachedTriangles.clear();
    m_CachedBVHNodes.clear();
    m_CachedDiffuseTexIDs.clear();
    m_CachedSpecularTexIDs.clear();
    m_RTGeoDirty = true;
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
