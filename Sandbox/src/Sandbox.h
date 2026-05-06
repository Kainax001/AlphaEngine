#pragma once
#include <AlphaApplication/AlphaApplication.h>
#include <AlphaGraphic/AlphaGraphic.h>
#include <glm/gtc/matrix_transform.hpp>

class Sandbox : public AAP::Application {
protected:
    bool OnInit() override;
    void OnShutdown() override;
    void OnUpdate(float dt) override;
    void OnRender() override;

private:
    AG::Camera m_Camera;
    AG::Shader m_Shader;
    AG::Model m_Model;
    AG::DirectionalLight m_DirLight;
    AG::PointLight m_PointLight;
};
