#pragma once
#include <AlphaGraphic/AlphaGraphic.h>
#include "Component.h"
#include <memory>
#include <string>
#include <typeindex>
#include <vector>
#include <algorithm>
#include <utility>
#include <rapidjson/document.h>

namespace AC { class Input; }

namespace AS {

class Actor {
public:
    explicit Actor(std::string name);
    ~Actor();

    void BeginPlay();
    void Tick(float dt);
    void FixedTick(float dt);
    void EndPlay();

    // Add a component. Multiple components of the same type are allowed.
    // Insertion order = Tick execution order.
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args)
    {
        auto comp = std::make_unique<T>(this, std::forward<Args>(args)...);
        T* ptr    = comp.get();
        m_Components.push_back(std::move(comp));
        return ptr;
    }

    // For deserialization: append a pre-constructed component.
    void AddComponentRaw(std::unique_ptr<Component> comp);

    // Returns the first component of type T, or nullptr.
    template<typename T>
    T* GetComponent() const
    {
        const std::type_index target(typeid(T));
        for (const auto& comp : m_Components)
            if (comp->GetTypeIndex() == target)
                return static_cast<T*>(comp.get());
        return nullptr;
    }

    // Returns all components of type T.
    template<typename T>
    std::vector<T*> GetComponents() const
    {
        std::vector<T*> result;
        const std::type_index target(typeid(T));
        for (const auto& comp : m_Components)
            if (comp->GetTypeIndex() == target)
                result.push_back(static_cast<T*>(comp.get()));
        return result;
    }

    template<typename T>
    bool HasComponent() const
    {
        const std::type_index target(typeid(T));
        for (const auto& comp : m_Components)
            if (comp->GetTypeIndex() == target) return true;
        return false;
    }

    // Remove the first component of type T.
    template<typename T>
    void RemoveComponent()
    {
        const std::type_index target(typeid(T));
        for (auto it = m_Components.begin(); it != m_Components.end(); ++it)
        {
            if ((*it)->GetTypeIndex() == target)
            {
                (*it)->EndPlay();
                m_Components.erase(it);
                return;
            }
        }
    }

    // Remove all components of type T.
    template<typename T>
    void RemoveComponents()
    {
        const std::type_index target(typeid(T));
        for (int i = static_cast<int>(m_Components.size()) - 1; i >= 0; --i)
        {
            if (m_Components[i]->GetTypeIndex() == target)
            {
                m_Components[i]->EndPlay();
                m_Components.erase(m_Components.begin() + i);
            }
        }
    }

    AG::Transform&       GetTransform()       { return m_Transform; }
    const AG::Transform& GetTransform() const { return m_Transform; }
    const std::string&   GetName()      const { return m_Name; }
    bool                 IsActive()     const { return m_bActive; }
    void                 SetActive(bool v)    { m_bActive = v; }

    void Serialize  (rapidjson::Value& out,
                     rapidjson::Document::AllocatorType& alloc) const;
    void Deserialize(const rapidjson::Value& in, AC::Input* input = nullptr);

private:
    std::string   m_Name;
    bool          m_bActive = true;
    AG::Transform m_Transform;
    std::vector<std::unique_ptr<Component>> m_Components;
};

} // namespace AS
