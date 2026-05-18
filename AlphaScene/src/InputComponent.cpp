#include "AlphaScene/InputComponent.h"
#include <cassert>

namespace AS {

InputComponent::InputComponent(Actor* owner)
    : Component(owner), m_Input(nullptr)
{
}

InputComponent::InputComponent(Actor* owner, AC::Input& input)
    : Component(owner), m_Input(&input)
{
}

const AC::Keyboard& InputComponent::GetKeyboard() const
{
    assert(m_Input && "InputComponent: no Input assigned");
    return m_Input->GetKeyboard();
}

const AC::Mouse& InputComponent::GetMouse() const
{
    assert(m_Input && "InputComponent: no Input assigned");
    return m_Input->GetMouse();
}

} // namespace AS
