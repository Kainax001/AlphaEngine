#pragma once

#include "AlphaGraphic/Shader/Shader.h"
#include <unordered_map>
#include <memory>

namespace AG {

// 컴파일된 셰이더를 이름으로 캐싱·관리하는 정적 저장소.
// 같은 셰이더를 여러 곳에서 참조해도 GL 프로그램 객체는 하나만 존재한다.
class ShaderLibrary
{
public:
    // 셰이더를 컴파일해 라이브러리에 등록한다.
    // 같은 name이 이미 등록되어 있으면 재컴파일 없이 캐시를 반환한다.
    static Shader& Load(const std::string& name,
                        const char* vertexPath,
                        const char* fragmentPath,
                        const char* geometryPath = nullptr);

    // 등록된 셰이더를 이름으로 가져온다.
    // 없는 이름을 요청하면 stderr에 에러를 출력하고 dangling ref를 반환하므로
    // 반드시 Exists()로 확인하거나 Load() 이후에만 호출해야 한다.
    static Shader& Get(const std::string& name);

    // -----------------------------------------------------------------------
    // Hot-Reload 인터페이스
    // -----------------------------------------------------------------------

    // 특정 셰이더 하나를 파일에서 다시 읽어 재컴파일한다.
    // 내부적으로 Shader::Reload()를 호출하므로
    // 컴파일 실패 시 이전 셰이더가 유지되고 false를 반환한다.
    //
    // 사용 예:
    //   if (input.IsKeyPressed(Key::F5))
    //       ShaderLibrary::Reload("Lit");
    static bool Reload(const std::string& name);

    // 라이브러리에 등록된 모든 셰이더를 일괄 재컴파일한다.
    // 각 셰이더마다 Reload()를 호출하므로 개별 실패가 나머지에 영향을 주지 않는다.
    //
    // 사용 예:
    //   if (input.IsKeyPressed(Key::R))
    //       ShaderLibrary::ReloadAll();
    static void ReloadAll();

    // -----------------------------------------------------------------------

    static bool Exists(const std::string& name);

    // 등록된 모든 셰이더를 해제한다. GL 프로그램도 함께 삭제된다.
    static void Clear();

private:
    // 셰이더를 name → unique_ptr<Shader> 로 보관한다.
    // unique_ptr이 소유권을 가지므로 Clear() 또는 프로그램 종료 시 자동 해제된다.
    static std::unordered_map<std::string, std::unique_ptr<Shader>> s_Shaders;
};

} // namespace AG
