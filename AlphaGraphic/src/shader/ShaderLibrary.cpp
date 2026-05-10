#include "AlphaGraphic/shader/ShaderLibrary.h"

#include <iostream>

namespace AG {

// 정적 멤버 정의. 프로그램 전체에서 하나의 맵만 존재한다.
std::unordered_map<std::string, std::unique_ptr<Shader>> ShaderLibrary::s_Shaders;

Shader& ShaderLibrary::Load(const std::string& name,
                             const char* vertexPath,
                             const char* fragmentPath,
                             const char* geometryPath)
{
    // 이미 등록된 이름이면 재컴파일 없이 캐시를 반환한다.
    // 같은 셰이더를 두 번 Load()해도 GL 프로그램이 중복 생성되지 않는다.
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

// ==============================================================================
// Hot-Reload
// ==============================================================================

bool ShaderLibrary::Reload(const std::string& name)
{
    // 등록되지 않은 이름은 Reload할 경로 정보가 없으므로 즉시 실패한다.
    if (!Exists(name))
    {
        std::cerr << "[AlphaGraphic] ERROR::SHADER_LIBRARY::Reload - not found: " << name << "\n";
        return false;
    }

    // Shader::Reload()에 실제 재컴파일을 위임한다.
    // 실패 시 Shader 내부에서 이전 GL 프로그램이 자동으로 복구된다.
    return s_Shaders[name]->Reload();
}

void ShaderLibrary::ReloadAll()
{
    // 등록된 셰이더를 순서 상관없이 전부 재컴파일한다.
    // 한 셰이더가 실패해도 루프가 계속되어 나머지 셰이더에는 영향을 주지 않는다.
    for (auto& [name, shader] : s_Shaders)
    {
        if (!shader->Reload())
            std::cerr << "[AlphaGraphic] ShaderLibrary::ReloadAll - failed: " << name << "\n";
    }
}

// ==============================================================================

bool ShaderLibrary::Exists(const std::string& name)
{
    return s_Shaders.find(name) != s_Shaders.end();
}

void ShaderLibrary::Clear()
{
    // unique_ptr 소멸자가 Shader 객체를 삭제하고,
    // Shader 소멸자(또는 명시적 glDeleteProgram)가 GL 프로그램을 해제한다.
    s_Shaders.clear();
}

} // namespace AG
