#include "AlphaControler/Mouse.h"
#include <GLFW/glfw3.h>
#include <cstring>

namespace AC {

Mouse::Mouse()
{
    memset(m_CurButtons, 0, sizeof(m_CurButtons));
    memset(m_PrevButtons, 0, sizeof(m_PrevButtons));
}

void Mouse::NewFrame()
{
    m_DeltaX = 0.0f;
    m_DeltaY = 0.0f;
    m_ScrollDelta = 0.0f;
}

void Mouse::Update(GLFWwindow* window)
{
    memcpy(m_PrevButtons, m_CurButtons, sizeof(m_CurButtons));

    for (int i = 0; i < BUTTON_COUNT; ++i)
        m_CurButtons[i] = glfwGetMouseButton(window, i) == GLFW_PRESS;

    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    m_X = static_cast<float>(mx);
    m_Y = static_cast<float>(my);
}

void Mouse::FeedDelta(float dx, float dy)
{
    m_DeltaX += dx;
    m_DeltaY += dy;
}

void Mouse::FeedScroll(float dy)
{
    m_ScrollDelta += dy;
}

bool Mouse::IsButtonDown(MouseButton btn) const
{
    int i = static_cast<int>(btn);
    return m_CurButtons[i];
}

bool Mouse::IsButtonPressed(MouseButton btn) const
{
    int i = static_cast<int>(btn);
    return m_CurButtons[i] && !m_PrevButtons[i];
}

bool Mouse::IsButtonReleased(MouseButton btn) const
{
    int i = static_cast<int>(btn);
    return !m_CurButtons[i] && m_PrevButtons[i];
}

} // namespace AC
