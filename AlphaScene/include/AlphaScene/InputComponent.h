#pragma once
#include "Component.h"
#include <AlphaControler/AlphaControler.h>

namespace AS {

// AC::Input, Keyboard, Mouse include is isolated to this component.
class InputComponent : public Component {
public:
    explicit InputComponent(Actor* owner);                     // for ComponentRegistry
    InputComponent(Actor* owner, AC::Input& input);           // normal construction

    void SetInput(AC::Input* input) { m_Input = input; }
    void SetInput(AC::Input& input) { m_Input = &input; }

    const char*     GetTypeName()  const override { return "InputComponent"; }
    std::type_index GetTypeIndex() const override { return typeid(InputComponent); }

    const AC::Keyboard& GetKeyboard() const;
    const AC::Mouse&    GetMouse()    const;

private:
    AC::Input* m_Input = nullptr;
};

} // namespace AS
