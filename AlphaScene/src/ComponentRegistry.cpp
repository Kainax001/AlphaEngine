#include "AlphaScene/ComponentRegistry.h"

namespace AS {

ComponentRegistry& ComponentRegistry::Get()
{
    static ComponentRegistry instance;
    return instance;
}

std::unique_ptr<Component> ComponentRegistry::Create(const std::string& typeName, Actor* owner)
{
    auto it = Get().m_Factories.find(typeName);
    if (it == Get().m_Factories.end()) return nullptr;
    return it->second(owner);
}

bool ComponentRegistry::Has(const std::string& typeName)
{
    return Get().m_Factories.count(typeName) > 0;
}

} // namespace AS
