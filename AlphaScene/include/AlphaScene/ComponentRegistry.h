#pragma once
#include "Component.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace AS {

class Actor;

class ComponentRegistry {
public:
    using Factory = std::function<std::unique_ptr<Component>(Actor*)>;

    // Usage: ComponentRegistry::Register<CameraComponent>("CameraComponent");
    template<typename T>
    static void Register(const std::string& typeName)
    {
        Get().m_Factories[typeName] = [](Actor* owner) {
            return std::make_unique<T>(owner);
        };
    }

    static std::unique_ptr<Component> Create(const std::string& typeName, Actor* owner);
    static bool Has(const std::string& typeName);

private:
    static ComponentRegistry& Get();
    std::unordered_map<std::string, Factory> m_Factories;
};

} // namespace AS
