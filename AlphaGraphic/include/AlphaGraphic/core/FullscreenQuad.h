#pragma once

#include <glad/glad.h>

namespace AG {

// Draws the result of a Compute Shader (stored in a GL_RGBA32F texture)
// as a fullscreen quad using the NDC big-triangle trick.
// Built-in vertex/fragment shaders are compiled from embedded source strings.
class FullscreenQuad
{
public:
    FullscreenQuad();
    ~FullscreenQuad();

    FullscreenQuad(const FullscreenQuad&)            = delete;
    FullscreenQuad& operator=(const FullscreenQuad&) = delete;

    // Bind textureID to slot 0 and draw 3 vertices (NDC triangle).
    void Draw(GLuint textureID) const;

    bool IsValid() const { return m_Program != 0; }

private:
    GLuint m_VAO     = 0;
    GLuint m_Program = 0;

    bool CompileBuiltinShaders();
    static GLuint CompileStage(GLenum type, const char* src);
};

} // namespace AG
