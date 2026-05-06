#pragma once
#include "MouseButton.h"

struct GLFWwindow;

namespace AC {

class Mouse {
public:
    Mouse();

    void NewFrame();
    void Update(GLFWwindow* window);

    void FeedDelta(float dx, float dy);
    void FeedScroll(float dy);

    bool IsButtonDown(MouseButton btn) const;
    bool IsButtonPressed(MouseButton btn) const;
    bool IsButtonReleased(MouseButton btn) const;

    float GetX() const { return m_X; }
    float GetY() const { return m_Y; }
    float GetDeltaX() const { return m_DeltaX; }
    float GetDeltaY() const { return m_DeltaY; }
    float GetScrollDelta() const { return m_ScrollDelta; }

private:
    static constexpr int BUTTON_COUNT = 8;
    bool m_CurButtons[BUTTON_COUNT] = {};
    bool m_PrevButtons[BUTTON_COUNT] = {};

    float m_X = 0.0f;
    float m_Y = 0.0f;
    float m_DeltaX = 0.0f;
    float m_DeltaY = 0.0f;
    float m_ScrollDelta = 0.0f;
};

} // namespace AC
