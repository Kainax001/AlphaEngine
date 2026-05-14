#include "AlphaScene/Actor.h"

namespace AS {

Actor::Actor(std::string name)
    : m_Name(std::move(name))
{
}

Actor::~Actor()
{
    EndPlay();
}

void Actor::BeginPlay()
{
    for (auto& [type, comp] : m_Components)
        comp->BeginPlay();
}

void Actor::Tick(float dt)
{
    if (!m_bActive) return;
    for (auto& [type, comp] : m_Components)
        comp->Tick(dt);
}

void Actor::FixedTick(float dt)
{
    if (!m_bActive) return;
    for (auto& [type, comp] : m_Components)
        comp->FixedTick(dt);
}

void Actor::EndPlay()
{
    for (auto& [type, comp] : m_Components)
        comp->EndPlay();
    m_Components.clear();
}

} // namespace AS
