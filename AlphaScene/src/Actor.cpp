#include "AlphaScene/Actor.h"
#include "AlphaScene/ComponentRegistry.h"
#include "AlphaScene/InputComponent.h"

#include <AlphaControler/AlphaControler.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace AS {

Actor::Actor(std::string name)
    : m_Name(std::move(name))
{
}

Actor::~Actor()
{
    EndPlay();
}

void Actor::BeginPlay()
{
    for (auto& comp : m_Components)
        comp->BeginPlay();
}

void Actor::Tick(float dt)
{
    if (!m_bActive) return;
    for (auto& comp : m_Components)
        comp->Tick(dt);
}

void Actor::FixedTick(float dt)
{
    if (!m_bActive) return;
    for (auto& comp : m_Components)
        comp->FixedTick(dt);
}

void Actor::EndPlay()
{
    for (auto& comp : m_Components)
        comp->EndPlay();
    m_Components.clear();
}

void Actor::AddComponentRaw(std::unique_ptr<Component> comp)
{
    m_Components.push_back(std::move(comp));
}

// ---------------------------------------------------------------------------
// Serialization helpers
// ---------------------------------------------------------------------------

static rapidjson::Value Vec3ToJson(const glm::vec3& v,
                                    rapidjson::Document::AllocatorType& alloc)
{
    rapidjson::Value arr(rapidjson::kArrayType);
    arr.PushBack(v.x, alloc);
    arr.PushBack(v.y, alloc);
    arr.PushBack(v.z, alloc);
    return arr;
}

// ---------------------------------------------------------------------------
// Serialize
// ---------------------------------------------------------------------------

void Actor::Serialize(rapidjson::Value& out,
                       rapidjson::Document::AllocatorType& alloc) const
{
    rapidjson::Value nameVal(rapidjson::kStringType);
    nameVal.SetString(m_Name.c_str(),
                      static_cast<rapidjson::SizeType>(m_Name.size()), alloc);
    out.AddMember("name",   nameVal,   alloc);
    out.AddMember("active", m_bActive, alloc);

    glm::vec3 pos   = m_Transform.GetPosition();
    glm::vec3 euler = glm::degrees(glm::eulerAngles(m_Transform.GetRotation()));
    glm::vec3 scale = m_Transform.GetScale();

    rapidjson::Value transform(rapidjson::kObjectType);
    transform.AddMember("position", Vec3ToJson(pos,   alloc), alloc);
    transform.AddMember("eulerDeg", Vec3ToJson(euler, alloc), alloc);
    transform.AddMember("scale",    Vec3ToJson(scale, alloc), alloc);
    out.AddMember("transform", transform, alloc);

    rapidjson::Value comps(rapidjson::kArrayType);
    for (const auto& comp : m_Components)
    {
        rapidjson::Value entry(rapidjson::kObjectType);

        rapidjson::Value typeVal(rapidjson::kStringType);
        const char* typeName = comp->GetTypeName();
        typeVal.SetString(typeName, alloc);
        entry.AddMember("type", typeVal, alloc);

        rapidjson::Value data(rapidjson::kObjectType);
        comp->Serialize(data, alloc);
        entry.AddMember("data", data, alloc);

        comps.PushBack(entry, alloc);
    }
    out.AddMember("components", comps, alloc);
}

// ---------------------------------------------------------------------------
// Deserialize
// ---------------------------------------------------------------------------

void Actor::Deserialize(const rapidjson::Value& in, AC::Input* input)
{
    if (in.HasMember("active") && in["active"].IsBool())
        m_bActive = in["active"].GetBool();

    if (in.HasMember("transform") && in["transform"].IsObject())
    {
        const auto& t = in["transform"];
        auto readVec3 = [](const rapidjson::Value& v) -> glm::vec3
        {
            if (!v.IsArray() || v.Size() < 3) return glm::vec3(0.0f);
            return glm::vec3(v[0].GetFloat(), v[1].GetFloat(), v[2].GetFloat());
        };

        if (t.HasMember("position")) m_Transform.SetPosition(readVec3(t["position"]));
        if (t.HasMember("eulerDeg")) m_Transform.SetEulerAngles(readVec3(t["eulerDeg"]));
        if (t.HasMember("scale"))    m_Transform.SetScale(readVec3(t["scale"]));
    }

    if (in.HasMember("components") && in["components"].IsArray())
    {
        for (const auto& entry : in["components"].GetArray())
        {
            if (!entry.HasMember("type") || !entry["type"].IsString()) continue;

            std::string typeName = entry["type"].GetString();
            auto comp = ComponentRegistry::Create(typeName, this);
            if (!comp) continue;

            if (entry.HasMember("data") && entry["data"].IsObject())
                comp->Deserialize(entry["data"]);

            if (typeName == "InputComponent" && input)
                static_cast<InputComponent*>(comp.get())->SetInput(input);

            AddComponentRaw(std::move(comp));
        }
    }
}

} // namespace AS
