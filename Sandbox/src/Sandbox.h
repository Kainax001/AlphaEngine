#pragma once
#include <AlphaApplication/AlphaApplication.h>
#include <AlphaScene/AlphaScene.h>
#include <cmath>

class Sandbox : public AAP::Application {
protected:
    void OnWorldInit(AS::World* world) override;
    void OnUpdate(float dt)            override;

private:
    // PointLight actor reference for orbit animation
    AS::Actor* m_PointLightActor = nullptr;
};
