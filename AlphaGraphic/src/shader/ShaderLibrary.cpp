#include "AlphaGraphic/shader/ShaderLibrary.h"

#include <iostream>

namespace AG {

std::unordered_map<std::string, std::unique_ptr<Shader>> ShaderLibrary::s_Shaders;

Shader& ShaderLibrary::Load(const std::string& name,
                             const char* vertexPath,
                             const char* fragmentPath,
                             const char* geometryPath)
{
    if (Exists(name))
    {
        std::cout << "[AlphaGraphic] ShaderLibrary: '" << name << "' already loaded, returning cached\n";
        return *s_Shaders[name];
    }

    s_Shaders[name] = std::make_unique<Shader>(vertexPath, fragmentPath, geometryPath);
    return *s_Shaders[name];
}

Shader& ShaderLibrary::Get(const std::string& name)
{
    if (!Exists(name))
        std::cerr << "[AlphaGraphic] ERROR::SHADER_LIBRARY::Shader not found: " << name << "\n";

    return *s_Shaders[name];
}

bool ShaderLibrary::Exists(const std::string& name)
{
    return s_Shaders.find(name) != s_Shaders.end();
}

void ShaderLibrary::Clear()
{
    s_Shaders.clear();
}

} // namespace AG