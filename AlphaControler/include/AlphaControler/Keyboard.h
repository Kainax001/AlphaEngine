#pragma once
#include "Key.h"

struct GLFWwindow;

namespace AC {

class Keyboard {
public:
    Keyboard();

    void NewFrame();
    void Update(GLFWwindow* window);

    bool IsKeyDown(Key key) const;
    bool IsKeyPressed(Key key) const;
    bool IsKeyReleased(Key key) const;

private:
    static constexpr int KEY_COUNT = 349;
    bool m_CurKeys[KEY_COUNT] = {};
    bool m_PrevKeys[KEY_COUNT] = {};
};

} // namespace AC