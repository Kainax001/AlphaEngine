#pragma once
#include <rapidjson/document.h>
#include <typeindex>

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

    // Serialization interface
    virtual const char*        GetTypeName()  const = 0;
    virtual std::type_index    GetTypeIndex() const = 0;

    virtual void Serialize  (rapidjson::Value& out,
                             rapidjson::Document::AllocatorType& alloc) const {}
    virtual void Deserialize(const rapidjson::Value& in) {}

    Actor* GetOwner() const { return m_Owner; }

protected:
    Actor* m_Owner = nullptr;
};

} // namespace AS
