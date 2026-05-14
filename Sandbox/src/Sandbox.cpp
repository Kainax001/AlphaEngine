#include "Sandbox.h"
#include <iostream>
#include <glm/glm.hpp>

void Sandbox::OnWorldInit(AS::World* world)
{
    std::cout << "[Sandbox] OnWorldInit\n";

    // Load shader into ShaderLibrary
    AG::ShaderLibrary::Load("Lit",
        SANDBOX_ASSET_DIR "shaders/Lit.vert",
        SANDBOX_ASSET_DIR "shaders/Lit.frag"
    );

    // ------------------------------------------------------------------
    // MainCamera
    // ------------------------------------------------------------------
    auto* camActor  = world->SpawnActor("MainCamera");
    auto* camComp   = camActor->AddComponent<AS::CameraComponent>();
    camComp->SetPosition(glm::vec3(0.0f, 1.0f, 5.0f));
    camComp->SetSpeed(4.0f);
    camActor->AddComponent<AS::InputComponent>(GetInput());

    // ------------------------------------------------------------------
    // Backpack
    // ------------------------------------------------------------------
    auto* backpack  = world->SpawnActor("Backpack");
    auto* mesh      = backpack->AddComponent<AS::MeshComponent>();
    mesh->LoadModel(SANDBOX_ASSET_DIR "models/backpack/backpack.obj");
    mesh->SetShaderName("Lit");

    // ------------------------------------------------------------------
    // Directional Light
    // ------------------------------------------------------------------
    auto* dirActor  = world->SpawnActor("DirLight");
    dirActor->AddComponent<AS::LightComponent>(
        std::make_shared<AG::DirectionalLight>(
            glm::vec3(-0.3f, -0.5f, -0.8f),
            glm::vec3(1.0f, 0.95f, 0.8f),
            1.0f
        )
    );

    // ------------------------------------------------------------------
    // Point Light  (position is animated in OnUpdate)
    // ------------------------------------------------------------------
    m_PointLightActor = world->SpawnActor("PointLight");
    m_PointLightActor->AddComponent<AS::LightComponent>(
        std::make_shared<AG::PointLight>(
            glm::vec3(3.0f, 3.0f, 4.0f),
            glm::vec3(1.0f, 0.9f, 0.8f),
            5.0f
        )
    );
    m_PointLightActor->GetTransform().SetPosition(glm::vec3(3.0f, 3.0f, 4.0f));
}

void Sandbox::OnUpdate(float dt)
{
    if (!m_PointLightActor) return;

    float t   = GetTime();
    float ang = t * 1.2f;
    glm::vec3 pos(std::cos(ang) * 4.0f, 3.0f, std::sin(ang) * 4.0f);

    // Update Actor transform — LightComponent reads position from Transform
    m_PointLightActor->GetTransform().SetPosition(pos);
}
