#include "AlphaWindow/WindowConfig.h"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <cstdio>
#include <iostream>

namespace AW {

WindowConfig WindowConfig::LoadFromFile(const std::string& path)
{
    WindowConfig cfg;

    FILE* fp = nullptr;
#ifdef _MSC_VER
    fopen_s(&fp, path.c_str(), "rb");
#else
    fp = fopen(path.c_str(), "rb");
#endif

    if (!fp)
    {
        std::cerr << "[WindowConfig] Cannot open '" << path << "' — using defaults\n";
        return cfg;
    }

    char buf[4096];
    rapidjson::FileReadStream is(fp, buf, sizeof(buf));

    rapidjson::Document doc;
    doc.ParseStream(is);
    fclose(fp);

    if (doc.HasParseError())
    {
        std::cerr << "[WindowConfig] JSON parse error in '" << path << "' — using defaults\n";
        return cfg;
    }

    if (doc.HasMember("title")          && doc["title"].IsString())  cfg.title          = doc["title"].GetString();
    if (doc.HasMember("width")          && doc["width"].IsInt())     cfg.width          = doc["width"].GetInt();
    if (doc.HasMember("height")         && doc["height"].IsInt())    cfg.height         = doc["height"].GetInt();
    if (doc.HasMember("vsync")          && doc["vsync"].IsBool())    cfg.vsync          = doc["vsync"].GetBool();
    if (doc.HasMember("fullscreen")     && doc["fullscreen"].IsBool())    cfg.fullscreen    = doc["fullscreen"].GetBool();
    if (doc.HasMember("capture_cursor") && doc["capture_cursor"].IsBool()) cfg.captureCursor = doc["capture_cursor"].GetBool();
    if (doc.HasMember("resizable")      && doc["resizable"].IsBool())     cfg.resizable     = doc["resizable"].GetBool();
    if (doc.HasMember("gl_major")       && doc["gl_major"].IsInt())       cfg.glMajor       = doc["gl_major"].GetInt();
    if (doc.HasMember("gl_minor")       && doc["gl_minor"].IsInt())       cfg.glMinor       = doc["gl_minor"].GetInt();

    std::cout << "[WindowConfig] Loaded from '" << path << "'\n";
    return cfg;
}

} // namespace AW
