#pragma once
#include <AlphaScene/AlphaScene.h>
#include "OrbitComponent.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>

// Convenience factory functions for common Actor setups.
// Keeps OnWorldInit concise without modifying the engine.
namespace Factory {

inline AS::Actor* Camera(AS::World* world, const std::string& name,
                          AC::Input& input,
                          glm::vec3 pos   = glm::vec3(0.0f),
                          float     speed = 4.0f)
{
    auto* actor = world->SpawnActor(name);
    auto* cam   = actor->AddComponent<AS::CameraComponent>();
    cam->SetPosition(pos);
    cam->SetSpeed(speed);
    actor->AddComponent<AS::InputComponent>(input);
    return actor;
}

inline AS::Actor* Mesh(AS::World* world, const std::string& name,
                        const std::string& modelPath,
                        const std::string& shaderName)
{
    auto* actor = world->SpawnActor(name);
    auto* mesh  = actor->AddComponent<AS::MeshComponent>();
    mesh->LoadModel(modelPath);
    mesh->SetShaderName(shaderName);
    return actor;
}

inline AS::Actor* DirLight(AS::World* world, const std::string& name,
                            glm::vec3 direction,
                            glm::vec3 color,
                            float     intensity = 1.0f)
{
    auto* actor = world->SpawnActor(name);
    actor->AddComponent<AS::LightComponent>(
        std::make_shared<AG::DirectionalLight>(direction, color, intensity));
    return actor;
}

inline AS::Actor* PointLight(AS::World* world, const std::string& name,
                              glm::vec3 position,
                              glm::vec3 color,
                              float     intensity = 1.0f)
{
    auto* actor = world->SpawnActor(name);
    actor->GetTransform().SetPosition(position);
    actor->AddComponent<AS::LightComponent>(
        std::make_shared<AG::PointLight>(position, color, intensity));
    return actor;
}

inline AS::Actor* SpotLight(AS::World* world, const std::string& name,
                             glm::vec3 position,
                             glm::vec3 direction,
                             glm::vec3 color,
                             float     intensity    = 1.0f,
                             float     cutOff       = 12.5f,
                             float     outerCutOff  = 17.5f)
{
    auto* actor = world->SpawnActor(name);
    actor->GetTransform().SetPosition(position);
    auto light = std::make_shared<AG::SpotLight>(position, direction, color, intensity);
    light->SetCutOff(cutOff);
    light->SetOuterCutOff(outerCutOff);
    actor->AddComponent<AS::LightComponent>(std::move(light));
    return actor;
}

} // namespace Factory
