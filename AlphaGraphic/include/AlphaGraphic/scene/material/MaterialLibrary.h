#pragma once

#include "AlphaGraphic/scene/material/Material.h"
#include <unordered_map>
#include <memory>
#include <string>

namespace AG {

class MaterialLibrary
{
public:
    static std::shared_ptr<Material> Load(const std::string& name, std::shared_ptr<Shader> shader);
    static std::shared_ptr<Material> Get   (const std::string& name);
    static bool                      Exists(const std::string& name);
    static void                      Clear();

private:
    static std::unordered_map<std::string, std::shared_ptr<Material>> s_Materials;
};

} // namespace AG
