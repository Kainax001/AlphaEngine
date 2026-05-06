#include "AlphaGraphic/core/Renderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace AG {

RendererConfig Renderer::s_Config;

// ============================================================
// Init / Shutdown
// ============================================================
bool Renderer::Init(const std::string& configPath)
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "[Renderer] Failed to initialize GLAD\n";
        return false;
    }

    if (!configPath.empty())
        s_Config = RendererConfig::LoadFromFile(configPath);

    ApplyConfig(s_Config);

    std::cout << "[Renderer] Initialized\n";
    return true;
}

void Renderer::Shutdown()
{
    std::cout << "[Renderer] Shutdown\n";
}

// ============================================================
// Frame
// ============================================================
void Renderer::BeginFrame()
{
    Clear();
}

void Renderer::EndFrame()
{
}

// ============================================================
// State
// ============================================================
void Renderer::SetClearColor(float r, float g, float b, float a)
{
    s_Config.clearColor[0] = r;
    s_Config.clearColor[1] = g;
    s_Config.clearColor[2] = b;
    s_Config.clearColor[3] = a;
    glClearColor(r, g, b, a);
}

void Renderer::Clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Renderer::ApplyConfig(const RendererConfig& cfg)
{
    // --- Clear ---
    glClearColor(cfg.clearColor[0], cfg.clearColor[1],
                 cfg.clearColor[2], cfg.clearColor[3]);
    glClearDepth(cfg.clearDepth);
    glClearStencil(cfg.clearStencil);

    // --- Depth ---
    cfg.depthTest ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
    glDepthFunc(ToDepthFunc(cfg.depthFunc));
    glDepthMask(cfg.depthMask ? GL_TRUE : GL_FALSE);

    // --- Blend ---
    cfg.blend ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
    glBlendFunc(ToBlendFactor(cfg.blendSrc), ToBlendFactor(cfg.blendDst));
    glBlendEquation(ToBlendEquation(cfg.blendEquation));

    // --- Cull ---
    cfg.cullFace ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    glCullFace(ToCullFace(cfg.cullMode));
    glFrontFace(ToFrontFace(cfg.frontFace));

    // --- Stencil ---
    cfg.stencilTest ? glEnable(GL_STENCIL_TEST) : glDisable(GL_STENCIL_TEST);
    glStencilFunc(ToStencilFunc(cfg.stencilFunc), cfg.stencilRef, cfg.stencilMask);
    glStencilOp(ToStencilOp(cfg.stencilSFail),
                ToStencilOp(cfg.stencilDpFail),
                ToStencilOp(cfg.stencilDpPass));

    // --- Polygon ---
    glPolygonMode(GL_FRONT_AND_BACK, ToPolygonMode(cfg.polygonMode));
    cfg.polygonOffsetFill ? glEnable(GL_POLYGON_OFFSET_FILL) : glDisable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(cfg.polygonOffsetFactor, cfg.polygonOffsetUnits);

    // --- Misc ---
    cfg.multisample     ? glEnable(GL_MULTISAMPLE)      : glDisable(GL_MULTISAMPLE);
    cfg.framebufferSRGB ? glEnable(GL_FRAMEBUFFER_SRGB) : glDisable(GL_FRAMEBUFFER_SRGB);
    cfg.scissorTest     ? glEnable(GL_SCISSOR_TEST)     : glDisable(GL_SCISSOR_TEST);
    glLineWidth(cfg.lineWidth);
    glPointSize(cfg.pointSize);
}

// ============================================================
// GLenum 변환 헬퍼
// ============================================================
unsigned int Renderer::ToDepthFunc(const std::string& s)
{
    if (s == "never")    return GL_NEVER;
    if (s == "less")     return GL_LESS;
    if (s == "equal")    return GL_EQUAL;
    if (s == "lequal")   return GL_LEQUAL;
    if (s == "greater")  return GL_GREATER;
    if (s == "notequal") return GL_NOTEQUAL;
    if (s == "gequal")   return GL_GEQUAL;
    if (s == "always")   return GL_ALWAYS;
    return GL_LESS;
}

unsigned int Renderer::ToBlendFactor(const std::string& s)
{
    if (s == "zero")                     return GL_ZERO;
    if (s == "one")                      return GL_ONE;
    if (s == "src_color")                return GL_SRC_COLOR;
    if (s == "one_minus_src_color")      return GL_ONE_MINUS_SRC_COLOR;
    if (s == "dst_color")                return GL_DST_COLOR;
    if (s == "one_minus_dst_color")      return GL_ONE_MINUS_DST_COLOR;
    if (s == "src_alpha")                return GL_SRC_ALPHA;
    if (s == "one_minus_src_alpha")      return GL_ONE_MINUS_SRC_ALPHA;
    if (s == "dst_alpha")                return GL_DST_ALPHA;
    if (s == "one_minus_dst_alpha")      return GL_ONE_MINUS_DST_ALPHA;
    if (s == "constant_color")           return GL_CONSTANT_COLOR;
    if (s == "one_minus_constant_color") return GL_ONE_MINUS_CONSTANT_COLOR;
    if (s == "constant_alpha")           return GL_CONSTANT_ALPHA;
    if (s == "one_minus_constant_alpha") return GL_ONE_MINUS_CONSTANT_ALPHA;
    if (s == "src_alpha_saturate")       return GL_SRC_ALPHA_SATURATE;
    return GL_ONE;
}

unsigned int Renderer::ToBlendEquation(const std::string& s)
{
    if (s == "add")              return GL_FUNC_ADD;
    if (s == "subtract")         return GL_FUNC_SUBTRACT;
    if (s == "reverse_subtract") return GL_FUNC_REVERSE_SUBTRACT;
    if (s == "min")              return GL_MIN;
    if (s == "max")              return GL_MAX;
    return GL_FUNC_ADD;
}

unsigned int Renderer::ToCullFace(const std::string& s)
{
    if (s == "front")          return GL_FRONT;
    if (s == "back")           return GL_BACK;
    if (s == "front_and_back") return GL_FRONT_AND_BACK;
    return GL_BACK;
}

unsigned int Renderer::ToFrontFace(const std::string& s)
{
    if (s == "cw")  return GL_CW;
    if (s == "ccw") return GL_CCW;
    return GL_CCW;
}

unsigned int Renderer::ToStencilFunc(const std::string& s)
{
    if (s == "never")    return GL_NEVER;
    if (s == "less")     return GL_LESS;
    if (s == "lequal")   return GL_LEQUAL;
    if (s == "equal")    return GL_EQUAL;
    if (s == "gequal")   return GL_GEQUAL;
    if (s == "greater")  return GL_GREATER;
    if (s == "notequal") return GL_NOTEQUAL;
    if (s == "always")   return GL_ALWAYS;
    return GL_ALWAYS;
}

unsigned int Renderer::ToStencilOp(const std::string& s)
{
    if (s == "keep")      return GL_KEEP;
    if (s == "zero")      return GL_ZERO;
    if (s == "replace")   return GL_REPLACE;
    if (s == "incr")      return GL_INCR;
    if (s == "incr_wrap") return GL_INCR_WRAP;
    if (s == "decr")      return GL_DECR;
    if (s == "decr_wrap") return GL_DECR_WRAP;
    if (s == "invert")    return GL_INVERT;
    return GL_KEEP;
}

unsigned int Renderer::ToPolygonMode(const std::string& s)
{
    if (s == "fill")  return GL_FILL;
    if (s == "line")  return GL_LINE;
    if (s == "point") return GL_POINT;
    return GL_FILL;
}

} // namespace AG
