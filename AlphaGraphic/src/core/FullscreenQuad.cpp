#include "AlphaGraphic/core/FullscreenQuad.h"

#include <iostream>

namespace AG {

// Big-triangle NDC trick: 3 vertices cover the full screen without a VBO.
static const char* s_VertSrc = R"GLSL(
#version 430 core
out vec2 v_TexCoord;
void main()
{
    vec2 uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    v_TexCoord  = uv;
    gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";

static const char* s_FragSrc = R"GLSL(
#version 430 core
in  vec2      v_TexCoord;
out vec4      FragColor;
uniform sampler2D u_Texture;
void main()
{
    FragColor = texture(u_Texture, v_TexCoord);
}
)GLSL";

FullscreenQuad::FullscreenQuad()
{
    // Empty VAO required by OpenGL Core Profile even without VBO data.
    glGenVertexArrays(1, &m_VAO);
    CompileBuiltinShaders();
}

FullscreenQuad::~FullscreenQuad()
{
    if (m_VAO     != 0) glDeleteVertexArrays(1, &m_VAO);
    if (m_Program != 0) glDeleteProgram(m_Program);
}

void FullscreenQuad::Draw(GLuint textureID) const
{
    if (m_Program == 0) return;

    GLint prevProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);

    glUseProgram(m_Program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(glGetUniformLocation(m_Program, "u_Texture"), 0);

    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glUseProgram(prevProgram);
}

bool FullscreenQuad::CompileBuiltinShaders()
{
    GLuint vert = CompileStage(GL_VERTEX_SHADER,   s_VertSrc);
    GLuint frag = CompileStage(GL_FRAGMENT_SHADER, s_FragSrc);

    if (vert == 0 || frag == 0)
    {
        if (vert) glDeleteShader(vert);
        if (frag) glDeleteShader(frag);
        return false;
    }

    m_Program = glCreateProgram();
    glAttachShader(m_Program, vert);
    glAttachShader(m_Program, frag);
    glLinkProgram(m_Program);

    GLint linked = 0;
    glGetProgramiv(m_Program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLchar log[512];
        glGetProgramInfoLog(m_Program, 512, nullptr, log);
        std::cerr << "[FullscreenQuad] Link error: " << log << "\n";
        glDeleteProgram(m_Program);
        m_Program = 0;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return m_Program != 0;
}

GLuint FullscreenQuad::CompileStage(GLenum type, const char* src)
{
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    GLint ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        GLchar log[512];
        glGetShaderInfoLog(id, 512, nullptr, log);
        std::cerr << "[FullscreenQuad] Shader compile error: " << log << "\n";
        glDeleteShader(id);
        return 0;
    }
    return id;
}

} // namespace AG
