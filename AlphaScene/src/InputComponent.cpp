#include "AlphaScene/InputComponent.h"

namespace AS {

InputComponent::InputComponent(Actor* owner, AC::Input& input)
    : Component(owner), m_Input(input)
{
}

const AC::Keyboard& InputComponent::GetKeyboard() const
{
    return m_Input.GetKeyboard();
}

const AC::Mouse& InputComponent::GetMouse() const
{
    return m_Input.GetMouse();
}

} // namespace AS
