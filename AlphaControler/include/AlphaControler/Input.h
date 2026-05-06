#pragma once
#include "Keyboard.h"
#include "Mouse.h"

struct GLFWwindow;

namespace AC {

class Input {
public:
    Input();

    void Init(GLFWwindow* window);

    void NewFrame();
    void Update();

    Keyboard& GetKeyboard() { return m_Keyboard; }
    const Keyboard& GetKeyboard() const { return m_Keyboard; }

    Mouse& GetMouse() { return m_Mouse; }
    const Mouse& GetMouse() const { return m_Mouse; }

    void FeedMouseDelta(float dx, float dy);
    void FeedScroll(float dy);

private:
    GLFWwindow* m_Window = nullptr;
    Keyboard m_Keyboard;
    Mouse m_Mouse;
};

} // namespace AC