#pragma once
#include "WindowConfig.h"
#include "WindowState.h"
#include <functional>
#include <string>

struct GLFWwindow;

namespace AW {

class Window {
public:
    using ResizeFn    = std::function<void(int, int)>;
    using KeyFn       = std::function<void(int key, int action, int mods)>;
    using MouseMoveFn = std::function<void(float dx, float dy)>;
    using ScrollFn    = std::function<void(float dy)>;

    Window()  = default;
    ~Window() { Shutdown(); }

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    // --- 생명주기 ---
    // Uninitialized -> Created
    bool Init(const std::string& configPath);
    // any → Destroyed
    void Shutdown();
    // Running/Created -> Closing
    void Close();

    // --- 루프 인터페이스 ---
    void PollEvents();    // Created 상태면 Running으로 자동 전이
    void SwapBuffers();

    // --- 상태 조회 ---
    WindowState         GetState()  const { return m_State;  }
    const WindowConfig& GetConfig() const { return m_Config; }
    int                 GetWidth()  const { return m_Config.width;  }
    int                 GetHeight() const { return m_Config.height; }
    bool                IsRunning() const;   // Created|Running|Minimized

    // --- 런타임 설정 (Created|Running 상태에서만 유효) ---
    void SetTitle(const std::string& title);
    void SetVSync(bool enabled);

    // --- 콜백 등록 ---
    void OnResize   (ResizeFn    fn) { m_ResizeFn    = fn; }
    void OnKey      (KeyFn       fn) { m_KeyFn       = fn; }
    void OnMouseMove(MouseMoveFn fn) { m_MouseMoveFn = fn; }
    void OnScroll   (ScrollFn    fn) { m_ScrollFn    = fn; }

    GLFWwindow* GetNativeHandle() const { return m_Handle; }

private:
    bool TransitionTo(WindowState next);

    // GLFW 정적 콜백 -> glfwGetWindowUserPointer(this) 로 디스패치
    static void S_Resize   (GLFWwindow*, int, int);
    static void S_Key      (GLFWwindow*, int, int, int, int);
    static void S_MouseMove(GLFWwindow*, double, double);
    static void S_Scroll   (GLFWwindow*, double, double);

    GLFWwindow*  m_Handle = nullptr;
    WindowState  m_State  = WindowState::AW_Uninitialized;
    WindowConfig m_Config;

    bool  m_FirstMouse = true;
    float m_LastX      = 0.0f;
    float m_LastY      = 0.0f;

    ResizeFn    m_ResizeFn;
    KeyFn       m_KeyFn;
    MouseMoveFn m_MouseMoveFn;
    ScrollFn    m_ScrollFn;
};

} // namespace AW
