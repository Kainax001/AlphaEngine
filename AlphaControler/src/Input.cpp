#include "AlphaControler/Input.h"

namespace AC {

Input::Input()
{
}

void Input::Init(GLFWwindow* window)
{
    m_Window = window;
}

void Input::NewFrame()
{
    m_Keyboard.NewFrame();
    m_Mouse.NewFrame();
}

void Input::Update()
{
    if (!m_Window)
        return;

    m_Keyboard.Update(m_Window);
    m_Mouse.Update(m_Window);
}

void Input::FeedMouseDelta(float dx, float dy)
{
    m_Mouse.FeedDelta(dx, dy);
}

void Input::FeedScroll(float dy)
{
    m_Mouse.FeedScroll(dy);
}

} // namespace AC
