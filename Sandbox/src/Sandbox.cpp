#include "Sandbox.h"
#include "ActorFactory.h"
#include <iostream>

void Sandbox::OnWorldInit(AS::World* world)
{
    std::cout << "[Sandbox] OnWorldInit\n";

    AG::ShaderLibrary::Load("Lit",
        SANDBOX_ASSET_DIR "shaders/Lit.vert",
        SANDBOX_ASSET_DIR "shaders/Lit.frag");

    world->EnableDefaultLighting(1);

    Factory::Camera   (world, "MainCamera", GetInput(), {0.0f, 1.0f, 5.0f}, 4.0f);
    Factory::Mesh     (world, "Backpack",   SANDBOX_ASSET_DIR "models/backpack/backpack.obj", "Lit");
    Factory::DirLight (world, "DirLight",   {-0.3f, -0.5f, -0.8f}, {1.0f, 0.95f, 0.8f}, 1.0f);
    Factory::SpotLight(world, "SpotLight",  {0.0f, 4.0f, 3.0f}, {0.0f, -1.0f, -0.8f}, {1.0f, 1.0f, 0.95f}, 3.0f);

    Factory::PointLight(world, "PointLight", {3.0f, 3.0f, 4.0f}, {1.0f, 0.9f, 0.8f}, 5.0f)
        ->AddComponent<OrbitComponent>(4.0f, 1.2f, 3.0f);

    world->SaveScene(SANDBOX_ASSET_DIR "scenes/main.json");
}

