#include "Sandbox.h"
#include <iostream>
#include <cmath>
#include <GLFW/glfw3.h>

bool Sandbox::OnInit()
{
    std::cout << "[Sandbox] GL " << glGetString(GL_VERSION)
              << "  " << glGetString(GL_RENDERER) << "\n";

    m_Camera = AG::Camera(glm::vec3(0.0f, 1.0f, 5.0f));
    m_Camera.SetSpeed(4.0f);

    m_Shader = AG::Shader(
        SANDBOX_ASSET_DIR "shaders/Lit.vert",
        SANDBOX_ASSET_DIR "shaders/Lit.frag"
    );

    m_Model = AG::Model(SANDBOX_ASSET_DIR "models/backpack/backpack.obj");
    m_Model.GetTransform().SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    m_Model.GetTransform().SetScale(glm::vec3(1.0f, 1.0f, 1.0f));

    m_DirLight = AG::DirectionalLight(
        glm::vec3(-0.3f, -0.5f, -0.8f),
        glm::vec3(1.0f, 0.95f, 0.8f),
        1.0f
    );

    m_PointLight = AG::PointLight(
        glm::vec3(3.0f, 3.0f, 4.0f),
        glm::vec3(1.0f, 0.9f, 0.8f),
        5.0f
    );

    // -----------------------------------------------------------------------
    // 조명 UBO 초기화 (binding = 1)
    // GL 컨텍스트 생성 이후이므로 여기서 생성한다.
    // -----------------------------------------------------------------------
    m_LightUBO = std::make_unique<AG::UBO>(sizeof(LightUBOData), 1);
    m_LightUBO->LinkToShader(m_Shader.m_ID, "LightBlock", 1);

    // 초기 데이터 설정 후 한 번 업로드
    m_LightData.dirDirection     = glm::vec4(m_DirLight.GetDirection(),  m_DirLight.GetIntensity());
    m_LightData.dirColor         = glm::vec4(m_DirLight.GetColor(),      0.0f);
    m_LightData.pointPosition    = glm::vec4(m_PointLight.GetPosition(), m_PointLight.GetIntensity());
    m_LightData.pointColor       = glm::vec4(m_PointLight.GetColor(),    m_PointLight.GetConstant());
    m_LightData.pointAttenuation = glm::vec4(m_PointLight.GetLinear(), m_PointLight.GetQuadratic(), 0.0f, 0.0f);

    m_LightUBO->UpdateData(0, sizeof(LightUBOData), &m_LightData);
    m_LightsDirty = false;

    return true;
}

void Sandbox::OnShutdown()
{
}

void Sandbox::OnUpdate(float dt)
{
    const AC::Keyboard& keyboard = GetInput().GetKeyboard();
    const AC::Mouse& mouse = GetInput().GetMouse();

    if (keyboard.IsKeyDown(AC::Key::W)) m_Camera.ProcessKeyboard(AG::CameraMovement::Forward, dt);
    if (keyboard.IsKeyDown(AC::Key::S)) m_Camera.ProcessKeyboard(AG::CameraMovement::Backward, dt);
    if (keyboard.IsKeyDown(AC::Key::A)) m_Camera.ProcessKeyboard(AG::CameraMovement::Left, dt);
    if (keyboard.IsKeyDown(AC::Key::D)) m_Camera.ProcessKeyboard(AG::CameraMovement::Right, dt);
    if (keyboard.IsKeyDown(AC::Key::E)) m_Camera.ProcessKeyboard(AG::CameraMovement::Up, dt);
    if (keyboard.IsKeyDown(AC::Key::Q)) m_Camera.ProcessKeyboard(AG::CameraMovement::Down, dt);

    m_Camera.ProcessMouseMove(mouse.GetDeltaX(), mouse.GetDeltaY());
    m_Camera.ProcessMouseScroll(mouse.GetScrollDelta());

    float now = GetTime();
    float plA = now * 1.2f;
    glm::vec3 plP = glm::vec3(std::cos(plA) * 4.0f, 3.0f, std::sin(plA) * 4.0f);
    m_PointLight.SetPosition(plP);

    // PointLight 위치가 바뀌었으므로 UBO를 다음 렌더에서 재업로드한다.
    m_LightData.pointPosition = glm::vec4(plP, m_PointLight.GetIntensity());
    m_LightsDirty = true;
}

void Sandbox::OnRender()
{
    // window 크기 고정일 경우 aspect 계산 생략하고 상수로 둘 수 있게 해놔야 함, 멤버변수 추가하자.
    int w = GetWindow().GetWidth();
    int h = GetWindow().GetHeight();
    float aspect = (h > 0) ? static_cast<float>(w) / static_cast<float>(h) : 1.0f;

    glm::mat4 view = m_Camera.GetViewMatrix();
    glm::mat4 proj = m_Camera.GetProjectionMatrix(aspect);
    glm::vec3 eye = m_Camera.GetPosition();

    // 조명 데이터가 변경됐을 때만 UBO를 재업로드한다 (Dirty Flag).
    if (m_LightsDirty)
    {
        m_LightUBO->UpdateData(0, sizeof(LightUBOData), &m_LightData);
        m_LightsDirty = false;
    }

    m_Shader.Use();
    m_Shader.SetMat4("u_View", view);
    m_Shader.SetMat4("u_Projection", proj);
    m_Shader.SetVec3("u_ViewPos", eye);

    m_Shader.SetInt("u_Diffuse", 0);
    m_Shader.SetMat4("u_Model", m_Model.GetTransform().GetModelMatrix());
    m_Model.Draw();
}
