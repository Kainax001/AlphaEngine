#pragma once

#include "AlphaGraphic/Shader/Shader.h"
#include <unordered_map>
#include <memory>

namespace AG {

class ShaderLibrary
{
public:
    static Shader& Load(const std::string& name,
                        const char* vertexPath,
                        const char* fragmentPath,
                        const char* geometryPath = nullptr);

    static Shader& Get(const std::string& name);

    static bool    Exists(const std::string& name);
    static void    Clear();

private:
    static std::unordered_map<std::string, std::unique_ptr<Shader>> s_Shaders;
};

} // namespace AG