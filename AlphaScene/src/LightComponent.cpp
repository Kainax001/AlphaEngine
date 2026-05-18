#include "AlphaScene/LightComponent.h"
#include "AlphaScene/Actor.h"
#include <glm/gtc/matrix_transform.hpp>

namespace AS {

LightComponent::LightComponent(Actor* owner)
    : Component(owner)
{
}

LightComponent::LightComponent(Actor* owner, std::shared_ptr<AG::Light> light)
    : Component(owner), m_Light(std::move(light))
{
}

LightData LightComponent::GetLightData() const
{
    LightData data;
    if (!m_Light) return data;

    data.color     = m_Light->GetColor();
    data.intensity = m_Light->GetIntensity();

    switch (m_Light->GetType())
    {
        case AG::LightType::Directional:
        {
            auto* dir      = static_cast<AG::DirectionalLight*>(m_Light.get());
            data.type      = LightType::Directional;
            data.direction = dir->GetDirection();
            break;
        }
        case AG::LightType::Point:
        {
            auto* pt      = static_cast<AG::PointLight*>(m_Light.get());
            data.type     = LightType::Point;
            data.position = m_Owner->GetTransform().GetPosition();
            data.constant  = pt->GetConstant();
            data.linear    = pt->GetLinear();
            data.quadratic = pt->GetQuadratic();
            break;
        }
        case AG::LightType::Spot:
        {
            auto* sp         = static_cast<AG::SpotLight*>(m_Light.get());
            data.type        = LightType::Spot;
            data.position    = m_Owner->GetTransform().GetPosition();
            data.direction   = sp->GetDirection();
            data.cutOff      = sp->GetCutOff();
            data.outerCutOff = sp->GetOuterCutOff();
            break;
        }
    }

    return data;
}

// ---------------------------------------------------------------------------
// Serialization
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

void LightComponent::Serialize(rapidjson::Value& out,
                                 rapidjson::Document::AllocatorType& alloc) const
{
    if (!m_Light) return;

    auto type = m_Light->GetType();

    if (type == AG::LightType::Directional)
    {
        out.AddMember("lightType", "Directional", alloc);
        auto* dl = static_cast<AG::DirectionalLight*>(m_Light.get());
        out.AddMember("direction", Vec3ToJson(dl->GetDirection(), alloc), alloc);
    }
    else if (type == AG::LightType::Point)
    {
        out.AddMember("lightType", "Point", alloc);
        auto* pl = static_cast<AG::PointLight*>(m_Light.get());
        out.AddMember("position",  Vec3ToJson(pl->GetPosition(), alloc), alloc);
        out.AddMember("constant",  pl->GetConstant(),  alloc);
        out.AddMember("linear",    pl->GetLinear(),    alloc);
        out.AddMember("quadratic", pl->GetQuadratic(), alloc);
    }
    else if (type == AG::LightType::Spot)
    {
        out.AddMember("lightType", "Spot", alloc);
        auto* sl = static_cast<AG::SpotLight*>(m_Light.get());
        out.AddMember("position",    Vec3ToJson(sl->GetPosition(), alloc), alloc);
        out.AddMember("direction",   Vec3ToJson(sl->GetDirection(), alloc), alloc);
        out.AddMember("cutOff",      sl->GetCutOff(),      alloc);
        out.AddMember("outerCutOff", sl->GetOuterCutOff(), alloc);
    }

    out.AddMember("color",     Vec3ToJson(m_Light->GetColor(), alloc), alloc);
    out.AddMember("intensity", m_Light->GetIntensity(), alloc);
}

static glm::vec3 JsonToVec3(const rapidjson::Value& v)
{
    if (!v.IsArray() || v.Size() < 3) return glm::vec3(0.0f);
    return glm::vec3(v[0].GetFloat(), v[1].GetFloat(), v[2].GetFloat());
}

void LightComponent::Deserialize(const rapidjson::Value& in)
{
    if (!in.HasMember("lightType") || !in["lightType"].IsString()) return;

    std::string lt = in["lightType"].GetString();
    glm::vec3 color(1.0f);
    float intensity = 1.0f;

    if (in.HasMember("color"))     color     = JsonToVec3(in["color"]);
    if (in.HasMember("intensity") && in["intensity"].IsNumber())
        intensity = in["intensity"].GetFloat();

    if (lt == "Directional")
    {
        glm::vec3 dir(0.0f, -1.0f, 0.0f);
        if (in.HasMember("direction")) dir = JsonToVec3(in["direction"]);
        m_Light = std::make_shared<AG::DirectionalLight>(dir, color, intensity);
    }
    else if (lt == "Point")
    {
        glm::vec3 pos(0.0f);
        if (in.HasMember("position")) pos = JsonToVec3(in["position"]);
        auto pl = std::make_shared<AG::PointLight>(pos, color, intensity);
        if (in.HasMember("constant")  && in["constant"].IsNumber())
            pl->SetAttenuation(in["constant"].GetFloat(),
                               in.HasMember("linear")    ? in["linear"].GetFloat()    : 0.09f,
                               in.HasMember("quadratic") ? in["quadratic"].GetFloat() : 0.032f);
        m_Light = std::move(pl);
    }
    else if (lt == "Spot")
    {
        glm::vec3 pos(0.0f), dir(0.0f, -1.0f, 0.0f);
        float cutOff = 12.5f, outerCutOff = 17.5f;
        if (in.HasMember("position"))    pos         = JsonToVec3(in["position"]);
        if (in.HasMember("direction"))   dir         = JsonToVec3(in["direction"]);
        if (in.HasMember("cutOff")      && in["cutOff"].IsNumber())      cutOff      = in["cutOff"].GetFloat();
        if (in.HasMember("outerCutOff") && in["outerCutOff"].IsNumber()) outerCutOff = in["outerCutOff"].GetFloat();
        auto sl = std::make_shared<AG::SpotLight>(pos, dir, color, intensity);
        sl->SetCutOff(cutOff);
        sl->SetOuterCutOff(outerCutOff);
        m_Light = std::move(sl);
    }
}

AG::LightProxy LightComponent::ToProxy() const
{
    AG::LightProxy proxy{};
    if (!m_Light) return proxy;

    glm::vec3 color     = m_Light->GetColor();
    float     intensity = m_Light->GetIntensity();
    proxy.color = glm::vec4(color, 0.0f);

    switch (m_Light->GetType())
    {
        case AG::LightType::Directional:
        {
            auto* dl = static_cast<AG::DirectionalLight*>(m_Light.get());
            proxy.position  = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f); // type=0
            proxy.direction = glm::vec4(dl->GetDirection(), intensity);
            break;
        }
        case AG::LightType::Point:
        {
            auto* pl = static_cast<AG::PointLight*>(m_Light.get());
            glm::vec3 pos   = m_Owner->GetTransform().GetPosition();
            proxy.position  = glm::vec4(pos, 1.0f); // type=1
            proxy.direction = glm::vec4(0.0f, 0.0f, 0.0f, intensity);
            proxy.attenuation = glm::vec4(
                pl->GetConstant(), pl->GetLinear(), pl->GetQuadratic(), 0.0f);
            break;
        }
        case AG::LightType::Spot:
        {
            auto* sl = static_cast<AG::SpotLight*>(m_Light.get());
            glm::vec3 pos   = m_Owner->GetTransform().GetPosition();
            proxy.position  = glm::vec4(pos, 2.0f); // type=2
            proxy.direction = glm::vec4(sl->GetDirection(), intensity);
            float cosInner  = glm::cos(glm::radians(sl->GetCutOff()));
            float cosOuter  = glm::cos(glm::radians(sl->GetOuterCutOff()));
            proxy.spotParams = glm::vec4(cosInner, cosOuter, 0.0f, 0.0f);
            break;
        }
    }

    return proxy;
}

} // namespace AS
