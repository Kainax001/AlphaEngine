#pragma once
#include <string>

namespace AG {

struct RendererConfig {

    // --- Clear ---
    float clearColor[4] = { 0.07f, 0.07f, 0.10f, 1.0f };
    float clearDepth    = 1.0f;
    int   clearStencil  = 0;

    // --- Depth ---
    bool        depthTest = true;
    std::string depthFunc = "less";   // less | lequal | equal | greater | notequal | gequal | always | never
    bool        depthMask = true;

    // --- Blend ---
    bool        blend        = false;
    std::string blendSrc     = "src_alpha";           // src_alpha | one | zero | ...
    std::string blendDst     = "one_minus_src_alpha"; // one_minus_src_alpha | one | zero | ...
    std::string blendEquation = "add";                // add | subtract | reverse_subtract | min | max

    // --- Cull ---
    bool        cullFace  = false;
    std::string cullMode  = "back";   // back | front | front_and_back
    std::string frontFace = "ccw";    // ccw | cw

    // --- Stencil ---
    bool        stencilTest  = false;
    std::string stencilFunc  = "always";  // always | never | less | lequal | equal | gequal | greater | notequal
    int         stencilRef   = 0;
    int         stencilMask  = 0xFF;
    std::string stencilSFail  = "keep";  // keep | zero | replace | incr | incr_wrap | decr | decr_wrap | invert
    std::string stencilDpFail = "keep";
    std::string stencilDpPass = "keep";

    // --- Polygon ---
    std::string polygonMode        = "fill";  // fill | line | point
    bool        polygonOffsetFill  = false;
    float       polygonOffsetFactor = 0.0f;
    float       polygonOffsetUnits  = 0.0f;

    // --- Misc ---
    bool  multisample     = true;
    bool  framebufferSRGB = false;
    bool  scissorTest     = false;
    float lineWidth       = 1.0f;
    float pointSize       = 1.0f;

    static RendererConfig LoadFromFile(const std::string& path);
};

} // namespace AG
