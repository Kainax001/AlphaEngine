#include "AlphaControler/Keyboard.h"
#include <GLFW/glfw3.h>
#include <cstring>

namespace AC {

Keyboard::Keyboard()
{
    memset(m_CurKeys, 0, sizeof(m_CurKeys));
    memset(m_PrevKeys, 0, sizeof(m_PrevKeys));
}

void Keyboard::NewFrame()
{
    // No-op: deltas are reset in Mouse, not Keyboard
}

void Keyboard::Update(GLFWwindow* window)
{
    memcpy(m_PrevKeys, m_CurKeys, sizeof(m_CurKeys));

    for (int i = 0; i < KEY_COUNT; ++i)
        m_CurKeys[i] = glfwGetKey(window, i) == GLFW_PRESS;
}

bool Keyboard::IsKeyDown(Key key) const
{
    int i = static_cast<int>(key);
    return m_CurKeys[i];
}

bool Keyboard::IsKeyPressed(Key key) const
{
    int i = static_cast<int>(key);
    return m_CurKeys[i] && !m_PrevKeys[i];
}

bool Keyboard::IsKeyReleased(Key key) const
{
    int i = static_cast<int>(key);
    return !m_CurKeys[i] && m_PrevKeys[i];
}

} // namespace AC
