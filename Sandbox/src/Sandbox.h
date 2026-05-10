#pragma once
#include <AlphaApplication/AlphaApplication.h>
#include <AlphaGraphic/AlphaGraphic.h>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

class Sandbox : public AAP::Application {
protected:
    bool OnInit() override;
    void OnShutdown() override;
    void OnUpdate(float dt) override;
    void OnRender() override;

private:
    // -----------------------------------------------------------------------
    // 조명 UBO 데이터 (std140 레이아웃 — 모두 vec4로 패딩 없이 정렬)
    // binding = 1 에 업로드되며, 여러 셰이더가 같은 블록을 공유할 수 있다.
    // -----------------------------------------------------------------------
    struct LightUBOData {
        glm::vec4 dirDirection;      // xyz = direction,  w = intensity
        glm::vec4 dirColor;          // xyz = color,      w = 0 (padding)
        glm::vec4 pointPosition;     // xyz = position,   w = intensity
        glm::vec4 pointColor;        // xyz = color,      w = constant
        glm::vec4 pointAttenuation;  // x = linear, y = quadratic, zw = 0 (padding)
    };

    AG::Camera m_Camera;
    AG::Shader m_Shader;
    AG::Model  m_Model;

    AG::DirectionalLight m_DirLight;
    AG::PointLight       m_PointLight;

    // 조명 UBO: GL 컨텍스트 생성 후 OnInit()에서 초기화한다.
    std::unique_ptr<AG::UBO> m_LightUBO;
    LightUBOData             m_LightData  {};
    bool                     m_LightsDirty = true;
};
