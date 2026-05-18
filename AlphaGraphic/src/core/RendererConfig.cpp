#include "AlphaGraphic/core/RendererConfig.h"

#include <AlphaUtil/Json.h>
#include <rapidjson/document.h>
#include <iostream>

namespace AG {

RendererConfig RendererConfig::LoadFromFile(const std::string& path)
{
    RendererConfig cfg;

    rapidjson::Document doc;
    if (!AU::Json::LoadFile(path, doc))
    {
        std::cerr << "[RendererConfig] Using defaults\n";
        return cfg;
    }

    // --- Clear ---
    if (doc.HasMember("clear") && doc["clear"].IsObject())
    {
        const auto& c = doc["clear"];
        if (c.HasMember("color") && c["color"].IsArray() && c["color"].Size() >= 4)
        {
            for (int i = 0; i < 4; ++i)
                cfg.clearColor[i] = c["color"][i].GetFloat();
        }
        if (c.HasMember("depth")   && c["depth"].IsNumber())  cfg.clearDepth   = c["depth"].GetFloat();
        if (c.HasMember("stencil") && c["stencil"].IsInt())   cfg.clearStencil = c["stencil"].GetInt();
    }

    // --- Depth ---
    if (doc.HasMember("depth") && doc["depth"].IsObject())
    {
        const auto& d = doc["depth"];
        if (d.HasMember("test") && d["test"].IsBool())       cfg.depthTest = d["test"].GetBool();
        if (d.HasMember("func") && d["func"].IsString())     cfg.depthFunc = d["func"].GetString();
        if (d.HasMember("mask") && d["mask"].IsBool())       cfg.depthMask = d["mask"].GetBool();
    }

    // --- Blend ---
    if (doc.HasMember("blend") && doc["blend"].IsObject())
    {
        const auto& b = doc["blend"];
        if (b.HasMember("enabled")  && b["enabled"].IsBool())   cfg.blend         = b["enabled"].GetBool();
        if (b.HasMember("src")      && b["src"].IsString())      cfg.blendSrc      = b["src"].GetString();
        if (b.HasMember("dst")      && b["dst"].IsString())      cfg.blendDst      = b["dst"].GetString();
        if (b.HasMember("equation") && b["equation"].IsString()) cfg.blendEquation = b["equation"].GetString();
    }

    // --- Cull ---
    if (doc.HasMember("cull") && doc["cull"].IsObject())
    {
        const auto& c = doc["cull"];
        if (c.HasMember("enabled")    && c["enabled"].IsBool())    cfg.cullFace  = c["enabled"].GetBool();
        if (c.HasMember("face")       && c["face"].IsString())     cfg.cullMode  = c["face"].GetString();
        if (c.HasMember("front_face") && c["front_face"].IsString()) cfg.frontFace = c["front_face"].GetString();
    }

    // --- Stencil ---
    if (doc.HasMember("stencil") && doc["stencil"].IsObject())
    {
        const auto& s = doc["stencil"];
        if (s.HasMember("test")   && s["test"].IsBool())    cfg.stencilTest  = s["test"].GetBool();
        if (s.HasMember("func")   && s["func"].IsString())  cfg.stencilFunc  = s["func"].GetString();
        if (s.HasMember("ref")    && s["ref"].IsInt())      cfg.stencilRef   = s["ref"].GetInt();
        if (s.HasMember("mask")   && s["mask"].IsInt())     cfg.stencilMask  = s["mask"].GetInt();
        if (s.HasMember("sfail")  && s["sfail"].IsString()) cfg.stencilSFail  = s["sfail"].GetString();
        if (s.HasMember("dpfail") && s["dpfail"].IsString()) cfg.stencilDpFail = s["dpfail"].GetString();
        if (s.HasMember("dppass") && s["dppass"].IsString()) cfg.stencilDpPass = s["dppass"].GetString();
    }

    // --- Polygon ---
    if (doc.HasMember("polygon") && doc["polygon"].IsObject())
    {
        const auto& p = doc["polygon"];
        if (p.HasMember("mode")          && p["mode"].IsString())   cfg.polygonMode         = p["mode"].GetString();
        if (p.HasMember("offset_fill")   && p["offset_fill"].IsBool())  cfg.polygonOffsetFill  = p["offset_fill"].GetBool();
        if (p.HasMember("offset_factor") && p["offset_factor"].IsNumber()) cfg.polygonOffsetFactor = p["offset_factor"].GetFloat();
        if (p.HasMember("offset_units")  && p["offset_units"].IsNumber())  cfg.polygonOffsetUnits  = p["offset_units"].GetFloat();
    }

    // --- Misc ---
    if (doc.HasMember("multisample")      && doc["multisample"].IsBool())      cfg.multisample     = doc["multisample"].GetBool();
    if (doc.HasMember("framebuffer_srgb") && doc["framebuffer_srgb"].IsBool()) cfg.framebufferSRGB = doc["framebuffer_srgb"].GetBool();
    if (doc.HasMember("scissor_test")     && doc["scissor_test"].IsBool())     cfg.scissorTest     = doc["scissor_test"].GetBool();
    if (doc.HasMember("line_width")       && doc["line_width"].IsNumber())     cfg.lineWidth       = doc["line_width"].GetFloat();
    if (doc.HasMember("point_size")       && doc["point_size"].IsNumber())     cfg.pointSize       = doc["point_size"].GetFloat();

    std::cout << "[RendererConfig] Loaded from '" << path << "'\n";
    return cfg;
}

} // namespace AG
