#pragma once
#include "RendererConfig.h"
#include <string>

namespace AG {

class Renderer
{
public:
    static bool Init(const std::string& configPath = "");
    static void Shutdown();

    // TODO: virtual로 변경 예정
    static void BeginFrame();
    static void EndFrame();

    static void SetClearColor(float r, float g, float b, float a = 1.0f);
    static void Clear();

    static void ApplyConfig(const RendererConfig& cfg);
    static const RendererConfig& GetConfig() { return s_Config; }

private:
    static RendererConfig s_Config;

    // 문자열 -> GLenum 변환 헬퍼
    static unsigned int ToDepthFunc   (const std::string& s);
    static unsigned int ToBlendFactor (const std::string& s);
    static unsigned int ToBlendEquation(const std::string& s);
    static unsigned int ToCullFace    (const std::string& s);
    static unsigned int ToFrontFace   (const std::string& s);
    static unsigned int ToStencilFunc (const std::string& s);
    static unsigned int ToStencilOp   (const std::string& s);
    static unsigned int ToPolygonMode (const std::string& s);
};

} // namespace AG
