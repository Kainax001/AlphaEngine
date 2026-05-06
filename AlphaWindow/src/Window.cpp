#include "AlphaWindow/Window.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace AW {

// ============================================================
// 상태 전이 테이블
// ============================================================
//  Uninitialized → Created
//  Created       → Running
//  Running       → Minimized | Closing
//  Minimized     → Running   | Closing
//  Closing       → Destroyed
// ============================================================

bool Window::TransitionTo(WindowState next)
{
    using S = WindowState;

    bool valid = false;
    switch (m_State)
    {
        case S::AW_Uninitialized: valid = (next == S::AW_Created);                           break;
        case S::AW_Created:       valid = (next == S::AW_Running);                           break;
        case S::AW_Running:       valid = (next == S::AW_Minimized || next == S::AW_Closing);   break;
        case S::AW_Minimized:     valid = (next == S::AW_Running   || next == S::AW_Closing);   break;
        case S::AW_Closing:       valid = (next == S::AW_Destroyed);                         break;
        default:               valid = false;                                           break;
    }

    if (!valid)
    {
        std::cerr << "[Window] Invalid transition: "
                  << static_cast<int>(m_State) << " → " << static_cast<int>(next) << "\n";
        return false;
    }

    m_State = next;
    return true;
}

// ============================================================
// 생명주기
// ============================================================

bool Window::Init(const std::string& configPath)
{
    if (m_State != WindowState::AW_Uninitialized)
    {
        std::cerr << "[Window] Init called on already-initialized window\n";
        return false;
    }

    m_Config = WindowConfig::LoadFromFile(configPath);

    if (!glfwInit())
    {
        std::cerr << "[Window] glfwInit failed\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, m_Config.glMajor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, m_Config.glMinor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, m_Config.resizable ? GLFW_TRUE : GLFW_FALSE);

    m_Handle = glfwCreateWindow(
        m_Config.width, m_Config.height,
        m_Config.title.c_str(),
        m_Config.fullscreen ? glfwGetPrimaryMonitor() : nullptr,
        nullptr
    );

    if (!m_Handle)
    {
        std::cerr << "[Window] glfwCreateWindow failed\n";
        glfwTerminate();
        return false;
    }

    // ===== OpenGL context 활성화 =====
    glfwMakeContextCurrent(m_Handle);

    // ===== GLAD 로드: OpenGL 함수 포인터 설정 =====
    // 이 호출이 없으면 glCreateShader, glBindBuffer 등의 함수를 사용할 수 없음
    if (!gladLoadGL()) {
        std::cerr << "[Window] gladLoadGL failed\n";
        glfwDestroyWindow(m_Handle);
        glfwTerminate();
        return false;
    }

    // ===== OpenGL 초기 설정 =====
    glViewport(0, 0, m_Config.width, m_Config.height);
    glEnable(GL_DEPTH_TEST);

    std::cout << "[Window] OpenGL context initialized\n";
    std::cout << "[Window] GL Version: " << glGetString(GL_VERSION) << "\n";
    std::cout << "[Window] GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";

    glfwSwapInterval(m_Config.vsync ? 1 : 0);

    if (m_Config.captureCursor)
        glfwSetInputMode(m_Handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // this 포인터를 콜백에서 꺼낼 수 있도록 등록
    glfwSetWindowUserPointer      (m_Handle, this);
    glfwSetFramebufferSizeCallback(m_Handle, S_Resize);
    glfwSetKeyCallback            (m_Handle, S_Key);
    glfwSetCursorPosCallback      (m_Handle, S_MouseMove);
    glfwSetScrollCallback         (m_Handle, S_Scroll);

    // 초기 마우스 위치
    m_LastX      = static_cast<float>(m_Config.width)  * 0.5f;
    m_LastY      = static_cast<float>(m_Config.height) * 0.5f;
    m_FirstMouse = true;

    TransitionTo(WindowState::AW_Created);
    std::cout << "[Window] Created (" << m_Config.width << "x" << m_Config.height
              << " \"" << m_Config.title << "\")\n";
    return true;
}

void Window::Shutdown()
{
    if (m_State == WindowState::AW_Destroyed || m_State == WindowState::AW_Uninitialized)
        return;

    if (m_Handle)
    {
        glfwDestroyWindow(m_Handle);
        m_Handle = nullptr;
    }
    glfwTerminate();

    m_State = WindowState::AW_Destroyed;
    std::cout << "[Window] Destroyed\n";
}

void Window::Close()
{
    if (m_State == WindowState::AW_Running || m_State == WindowState::AW_Minimized
        || m_State == WindowState::AW_Created)
    {
        glfwSetWindowShouldClose(m_Handle, GLFW_TRUE);
        TransitionTo(WindowState::AW_Closing);
    }
}

// ============================================================
// 루프 인터페이스
// ============================================================

void Window::PollEvents()
{
    // Created 상태면 루프 첫 프레임 -> Running으로 전이
    if (m_State == WindowState::AW_Created)
        TransitionTo(WindowState::AW_Running);

    glfwPollEvents();

    if (!m_Handle) return;

    // 최소화 상태 감지
    bool iconified = glfwGetWindowAttrib(m_Handle, GLFW_ICONIFIED) != 0;
    if (iconified && m_State == WindowState::AW_Running)
        TransitionTo(WindowState::AW_Minimized);
    else if (!iconified && m_State == WindowState::AW_Minimized)
        TransitionTo(WindowState::AW_Running);

    // 윈도우 닫기 요청 감지 (X 버튼 등)
    if (glfwWindowShouldClose(m_Handle))
        if (m_State != WindowState::AW_Closing && m_State != WindowState::AW_Destroyed)
            TransitionTo(WindowState::AW_Closing);
}

void Window::SwapBuffers()
{
    if (m_Handle)
        glfwSwapBuffers(m_Handle);
}

bool Window::IsRunning() const
{
    return m_State == WindowState::AW_Created
        || m_State == WindowState::AW_Running
        || m_State == WindowState::AW_Minimized;
}

// ============================================================
// 런타임 설정
// ============================================================

void Window::SetTitle(const std::string& title)
{
    m_Config.title = title;
    if (m_Handle)
        glfwSetWindowTitle(m_Handle, title.c_str());
}

void Window::SetVSync(bool enabled)
{
    m_Config.vsync = enabled;
    glfwSwapInterval(enabled ? 1 : 0);
}

// ============================================================
// GLFW 정적 콜백
// ============================================================

void Window::S_Resize(GLFWwindow* handle, int w, int h)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(handle));
    self->m_Config.width  = w;
    self->m_Config.height = h;
    glViewport(0, 0, w, h);
    if (self->m_ResizeFn) self->m_ResizeFn(w, h);
}

void Window::S_Key(GLFWwindow* handle, int key, int /*scancode*/, int action, int mods)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(handle));
    if (self->m_KeyFn) self->m_KeyFn(key, action, mods);
}

void Window::S_MouseMove(GLFWwindow* handle, double xpos, double ypos)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(handle));

    float x = static_cast<float>(xpos);
    float y = static_cast<float>(ypos);

    if (self->m_FirstMouse)
    {
        self->m_LastX      = x;
        self->m_LastY      = y;
        self->m_FirstMouse = false;
        return;
    }

    float dx =  (x - self->m_LastX);
    float dy = -(y - self->m_LastY);
    self->m_LastX = x;
    self->m_LastY = y;

    if (self->m_MouseMoveFn) self->m_MouseMoveFn(dx, dy);
}

void Window::S_Scroll(GLFWwindow* handle, double /*xoffset*/, double yoffset)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(handle));
    if (self->m_ScrollFn) self->m_ScrollFn(static_cast<float>(yoffset));
}

} // namespace AW
