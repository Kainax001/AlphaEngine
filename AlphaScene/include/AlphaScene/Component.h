#pragma once

namespace AS {

class Actor;

class Component {
public:
    explicit Component(Actor* owner) : m_Owner(owner) {}
    virtual ~Component() = default;

    Component(const Component&)            = delete;
    Component& operator=(const Component&) = delete;

    virtual void BeginPlay()         {}
    virtual void Tick(float dt)      {}
    virtual void FixedTick(float dt) {}
    virtual void EndPlay()           {}

    Actor* GetOwner() const { return m_Owner; }

protected:
    Actor* m_Owner = nullptr;
};

} // namespace AS
