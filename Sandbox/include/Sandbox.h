#pragma once
#include <AlphaApplication/AlphaApplication.h>
#include <AlphaScene/AlphaScene.h>

class Sandbox : public AAP::Application {
protected:
    void OnWorldInit(AS::World* world) override;
};
