#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

namespace AG {

class Shader
{
public:
    // GPU에 올라간 셰이더 프로그램의 OpenGL 핸들.
    // glUseProgram, glUniform* 등 모든 GL 호출에 이 ID를 사용한다.
    unsigned int m_ID;

    // 경로 없이 빈 셰이더 객체만 만들 때 사용. m_ID = 0 이므로 IsValid()가 false.
    Shader() : m_ID(0) {}

    // 파일 경로를 받아 즉시 컴파일·링크한다.
    // geometryPath는 생략 가능(nullptr = 지오메트리 셰이더 없음).
    // 내부적으로 경로를 m_VertPath/m_FragPath/m_GeoPath에 저장해두어
    // 나중에 Reload()가 같은 파일을 다시 읽을 수 있게 한다.
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);

    void Use() const;

    // -----------------------------------------------------------------------
    // Hot-Reload 인터페이스
    // -----------------------------------------------------------------------

    // 생성 시 저장해둔 파일 경로로 셰이더를 다시 컴파일·링크한다.
    //
    // 성공 흐름:
    //   1. 새 GL 프로그램 컴파일·링크
    //   2. 이전 GL 프로그램(oldID) glDeleteProgram으로 해제
    //   3. m_ID = 새 프로그램 ID, true 반환
    //
    // 실패 흐름 (파일 없음 / 문법 오류 등):
    //   1. 컴파일 실패 → m_ID = oldID 로 복구  ← 렌더링이 끊기지 않는 핵심
    //   2. false 반환, 에러 메시지 출력
    //
    // 직접 호출 예시:
    //   if (input.IsKeyPressed(Key::F5))
    //       shader.Reload();
    bool Reload();

    // m_ID가 유효한 GL 프로그램인지 확인.
    // Reload() 실패 여부 체크나 초기화 전 guard로 사용한다.
    bool IsValid() const { return m_ID != 0; }

    // Reload()가 파일을 다시 열 수 있도록 저장해둔 경로 접근자.
    // ShaderLibrary::Reload() 등 외부에서 경로를 참조할 때도 사용한다.
    const std::string& GetVertPath() const { return m_VertPath; }
    const std::string& GetFragPath() const { return m_FragPath; }
    const std::string& GetGeoPath()  const { return m_GeoPath;  }

    // -----------------------------------------------------------------------
    // Uniform 세터 (핫-리로드와 무관, 셰이더 사용 인터페이스)
    // -----------------------------------------------------------------------
    void SetBool (const std::string& name, bool value)  const;
    void SetInt  (const std::string& name, int value)   const;
    void SetFloat(const std::string& name, float value) const;

    void SetVec2(const std::string& name, const glm::vec2& v)                 const;
    void SetVec2(const std::string& name, float x, float y)                   const;
    void SetVec3(const std::string& name, const glm::vec3& v)                 const;
    void SetVec3(const std::string& name, float x, float y, float z)          const;
    void SetVec4(const std::string& name, const glm::vec4& v)                 const;
    void SetVec4(const std::string& name, float x, float y, float z, float w) const;

    void SetMat2(const std::string& name, const glm::mat2& m) const;
    void SetMat3(const std::string& name, const glm::mat3& m) const;
    void SetMat4(const std::string& name, const glm::mat4& m) const;

private:
    // Reload()가 같은 파일 경로로 재컴파일할 수 있도록 생성자에서 저장한다.
    // 지오메트리 셰이더가 없으면 m_GeoPath는 빈 문자열.
    std::string m_VertPath;
    std::string m_FragPath;
    std::string m_GeoPath;

    // 실제 컴파일·링크 로직. 생성자와 Reload() 양쪽에서 호출된다.
    // 성공 시 m_ID에 새 GL 프로그램 ID를 쓰고 true 반환.
    // 실패 시 m_ID를 건드리지 않고 false 반환 → Reload()가 oldID를 복구한다.
    bool Compile(const std::string& vertPath, const std::string& fragPath,
                 const std::string& geoPath);

    void checkCompileErrors(unsigned int shader, const std::string& type);
};

} // namespace AG
