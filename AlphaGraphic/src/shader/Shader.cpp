#include "AlphaGraphic/shader/Shader.h"

#include <fstream>
#include <sstream>
#include <iostream>

namespace AG {

// ==============================================================================
// Constructor
// ==============================================================================
Shader::Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath)
{
    // 나중에 Reload()가 동일한 파일을 다시 열 수 있도록 경로를 멤버에 저장한다.
    // nullptr이면 빈 문자열로 초기화(지오메트리 셰이더 미사용 표시).
    m_VertPath = vertexPath   ? vertexPath   : "";
    m_FragPath = fragmentPath ? fragmentPath : "";
    m_GeoPath  = geometryPath ? geometryPath : "";

    Compile(m_VertPath, m_FragPath, m_GeoPath);
}

// ==============================================================================
// Reload
// ==============================================================================
bool Shader::Reload()
{
    // 경로가 없으면 어떤 파일을 읽어야 할지 알 수 없으므로 즉시 실패.
    // 기본 생성자(Shader())로 만든 객체가 이 케이스에 해당한다.
    if (m_VertPath.empty() || m_FragPath.empty())
    {
        std::cerr << "[AlphaGraphic] ERROR::SHADER::Reload failed - no paths stored\n";
        return false;
    }

    // 현재 정상 동작 중인 GL 프로그램 ID를 임시로 보관한다.
    // 재컴파일에 실패했을 때 이 값으로 m_ID를 복구해 렌더링이 끊기지 않게 한다.
    unsigned int oldID = m_ID;

    // Compile()이 m_ID에 새 값을 쓰기 전에 0으로 비워둔다.
    // Compile() 내부에서 실패하더라도 m_ID = 0 상태로 남고,
    // 아래 실패 분기에서 oldID를 다시 넣는다.
    m_ID = 0;

    if (!Compile(m_VertPath, m_FragPath, m_GeoPath))
    {
        // 컴파일·링크 실패 → 이전 GL 프로그램으로 되돌린다.
        // 이 덕분에 셰이더 파일에 문법 오류가 있어도 화면이 깨지지 않는다.
        m_ID = oldID;
        return false;
    }

    // 재컴파일 성공 → 이제 oldID는 GPU에서 더 이상 필요 없으므로 해제한다.
    // oldID가 0이면(기본 생성자로 만든 첫 컴파일) glDeleteProgram을 건너뛴다.
    if (oldID)
        glDeleteProgram(oldID);

    return true;
}

// ==============================================================================
// Use
// ==============================================================================
void Shader::Use() const
{
    glUseProgram(m_ID);
}

// ==============================================================================
// Compile (internal)
// 생성자와 Reload() 양쪽에서 호출되는 실제 컴파일·링크 로직.
// 성공 시 m_ID에 새 GL 프로그램 ID를 쓰고 true를 반환한다.
// 실패 시 m_ID를 건드리지 않고 false를 반환한다.
// ==============================================================================
bool Shader::Compile(const std::string& vertPath, const std::string& fragPath,
                     const std::string& geoPath)
{
    // GLAD가 아직 초기화되지 않은 상태에서 호출되면 glCreateShader가 nullptr이다.
    // OpenGL 컨텍스트 생성 전에 Shader를 만들려 했을 때 발생한다.
    if (!glCreateShader)
    {
        std::cerr << "[AlphaGraphic] ERROR::SHADER::GLAD not loaded\n";
        return false;
    }

    // ── 파일 읽기 ──────────────────────────────────────────────────────────
    std::string vertCode, fragCode, geoCode;
    try
    {
        // ifstream::failbit | badbit를 exception으로 처리해
        // 파일을 찾지 못하거나 읽기 오류가 생기면 catch로 넘어간다.
        auto readFile = [](const std::string& path) {
            std::ifstream f;
            f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            f.open(path);
            std::stringstream ss;
            ss << f.rdbuf();
            return ss.str();
        };

        vertCode = readFile(vertPath);
        fragCode = readFile(fragPath);

        // 지오메트리 셰이더는 경로가 있을 때만 읽는다.
        if (!geoPath.empty())
            geoCode = readFile(geoPath);
    }
    catch (std::ifstream::failure& e)
    {
        std::cerr << "[AlphaGraphic] ERROR::SHADER::FILE_NOT_FOUND: " << e.what() << "\n";
        return false;
    }

    // ── 스테이지별 컴파일 ──────────────────────────────────────────────────
    // 람다로 반복 로직을 묶어 vert/frag/geo 세 스테이지를 동일한 방식으로 처리한다.
    auto compileStage = [&](GLenum type, const std::string& code,
                            const std::string& typeName) -> unsigned int {
        const char* src = code.c_str();
        unsigned int id = glCreateShader(type);
        glShaderSource(id, 1, &src, nullptr);
        glCompileShader(id);
        checkCompileErrors(id, typeName); // 컴파일 오류를 stderr에 출력
        return id;
    };

    unsigned int vert = compileStage(GL_VERTEX_SHADER,   vertCode, "VERTEX");
    unsigned int frag = compileStage(GL_FRAGMENT_SHADER, fragCode, "FRAGMENT");
    unsigned int geo  = 0;
    if (!geoCode.empty())
        geo = compileStage(GL_GEOMETRY_SHADER, geoCode, "GEOMETRY");

    // ── 링크 ───────────────────────────────────────────────────────────────
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    if (geo) glAttachShader(prog, geo);
    glLinkProgram(prog);

    // 링크 성공 여부를 GL에서 직접 조회한다.
    GLint success = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);

    // 스테이지 셰이더 객체는 프로그램에 붙은 순간부터 프로그램이 소유한다.
    // 여기서 삭제해도 프로그램은 계속 동작한다.
    glDeleteShader(vert);
    glDeleteShader(frag);
    if (geo) glDeleteShader(geo);

    if (!success)
    {
        // 링크 실패 시 프로그램도 즉시 해제하고 false를 반환한다.
        // Reload() 호출 맥락에서는 m_ID가 아직 0이므로 oldID 복구 로직이 작동한다.
        GLchar log[1024];
        glGetProgramInfoLog(prog, 1024, nullptr, log);
        std::cerr << "[AlphaGraphic] ERROR::SHADER::LINK_FAILED\n" << log << "\n";
        glDeleteProgram(prog);
        return false;
    }

    // 성공 → m_ID에 새 프로그램 ID를 기록한다.
    m_ID = prog;
    return true;
}

// ==============================================================================
// Uniform setters
// ==============================================================================
void Shader::SetBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(m_ID, name.c_str()), (int)value);
}
void Shader::SetInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(m_ID, name.c_str()), value);
}
void Shader::SetFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(m_ID, name.c_str()), value);
}
void Shader::SetVec2(const std::string& name, const glm::vec2& v) const
{
    glUniform2fv(glGetUniformLocation(m_ID, name.c_str()), 1, &v[0]);
}
void Shader::SetVec2(const std::string& name, float x, float y) const
{
    glUniform2f(glGetUniformLocation(m_ID, name.c_str()), x, y);
}
void Shader::SetVec3(const std::string& name, const glm::vec3& v) const
{
    glUniform3fv(glGetUniformLocation(m_ID, name.c_str()), 1, &v[0]);
}
void Shader::SetVec3(const std::string& name, float x, float y, float z) const
{
    glUniform3f(glGetUniformLocation(m_ID, name.c_str()), x, y, z);
}
void Shader::SetVec4(const std::string& name, const glm::vec4& v) const
{
    glUniform4fv(glGetUniformLocation(m_ID, name.c_str()), 1, &v[0]);
}
void Shader::SetVec4(const std::string& name, float x, float y, float z, float w) const
{
    glUniform4f(glGetUniformLocation(m_ID, name.c_str()), x, y, z, w);
}
void Shader::SetMat2(const std::string& name, const glm::mat2& m) const
{
    glUniformMatrix2fv(glGetUniformLocation(m_ID, name.c_str()), 1, GL_FALSE, &m[0][0]);
}
void Shader::SetMat3(const std::string& name, const glm::mat3& m) const
{
    glUniformMatrix3fv(glGetUniformLocation(m_ID, name.c_str()), 1, GL_FALSE, &m[0][0]);
}
void Shader::SetMat4(const std::string& name, const glm::mat4& m) const
{
    glUniformMatrix4fv(glGetUniformLocation(m_ID, name.c_str()), 1, GL_FALSE, &m[0][0]);
}

// ==============================================================================
// Private
// ==============================================================================
void Shader::checkCompileErrors(unsigned int shader, const std::string& type)
{
    GLint  success;
    GLchar infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        std::cerr << "[AlphaGraphic] ERROR::SHADER::" << type << "_COMPILE\n" << infoLog << "\n";
    }
}

} // namespace AG
