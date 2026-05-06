#pragma once
#include <string>

namespace AW {

struct WindowConfig {
    std::string title         = "AlphaEngine";
    int         width         = 1280;
    int         height        = 720;
    bool        vsync         = true;
    bool        fullscreen    = false;
    bool        captureCursor = true;
    bool        resizable     = true;
    int         glMajor       = 3;
    int         glMinor       = 3;

    static WindowConfig LoadFromFile(const std::string& path);
};

} // namespace AW
