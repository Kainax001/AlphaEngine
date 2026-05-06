#include "AlphaGraphic/scene/material/MaterialLibrary.h"

#include <iostream>

namespace AG {

std::unordered_map<std::string, std::shared_ptr<Material>> MaterialLibrary::s_Materials;

std::shared_ptr<Material> MaterialLibrary::Load(const std::string& name, std::shared_ptr<Shader> shader)
{
    auto mat = std::make_shared<Material>(shader);
    s_Materials[name] = mat;
    return mat;
}

std::shared_ptr<Material> MaterialLibrary::Get(const std::string& name)
{
    auto it = s_Materials.find(name);
    if (it == s_Materials.end())
    {
        std::cerr << "[AlphaGraphic] ERROR::MATERIAL_LIBRARY::Material not found: " << name << "\n";
        return nullptr;
    }
    return it->second;
}

bool MaterialLibrary::Exists(const std::string& name)
{
    return s_Materials.count(name) > 0;
}

void MaterialLibrary::Clear()
{
    s_Materials.clear();
}

} // namespace AG
