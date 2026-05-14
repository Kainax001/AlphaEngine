#pragma once
#include <AlphaGraphic/AlphaGraphic.h>
#include "Component.h"
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace AS {

class Actor {
public:
    explicit Actor(std::string name);
    ~Actor();

    void BeginPlay();
    void Tick(float dt);
    void FixedTick(float dt);
    void EndPlay();

    template<typename T, typename... Args>
    T* AddComponent(Args&&... args)
    {
        auto comp = std::make_unique<T>(this, std::forward<Args>(args)...);
        T* ptr = comp.get();
        m_Components[std::type_index(typeid(T))] = std::move(comp);
        return ptr;
    }

    template<typename T>
    T* GetComponent() const
    {
        auto it = m_Components.find(std::type_index(typeid(T)));
        return (it != m_Components.end())
               ? static_cast<T*>(it->second.get())
               : nullptr;
    }

    template<typename T>
    bool HasComponent() const
    {
        return m_Components.count(std::type_index(typeid(T))) > 0;
    }

    template<typename T>
    void RemoveComponent()
    {
        m_Components.erase(std::type_index(typeid(T)));
    }

    AG::Transform&       GetTransform()       { return m_Transform; }
    const AG::Transform& GetTransform() const { return m_Transform; }
    const std::string&   GetName()      const { return m_Name; }
    bool                 IsActive()     const { return m_bActive; }
    void                 SetActive(bool v)    { m_bActive = v; }

private:
    std::string   m_Name;
    bool          m_bActive = true;
    AG::Transform m_Transform;
    std::unordered_map<std::type_index, std::unique_ptr<Component>> m_Components;
};

} // namespace AS
