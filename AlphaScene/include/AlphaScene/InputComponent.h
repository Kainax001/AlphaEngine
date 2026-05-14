#pragma once
#include "Component.h"
#include <AlphaControler/AlphaControler.h>

namespace AS {

// AC::Input, Keyboard, Mouse include is isolated to this component.
class InputComponent : public Component {
public:
    InputComponent(Actor* owner, AC::Input& input);

    const AC::Keyboard& GetKeyboard() const;
    const AC::Mouse&    GetMouse()    const;

private:
    AC::Input& m_Input;
};

} // namespace AS
