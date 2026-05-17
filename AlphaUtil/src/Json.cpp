#include "AlphaUtil/Json.h"

#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <cstdio>
#include <iostream>

namespace AU {

bool Json::LoadFile(const std::string& path, rapidjson::Document& out)
{
    FILE* fp = nullptr;
#ifdef _MSC_VER
    fopen_s(&fp, path.c_str(), "rb");
#else
    fp = fopen(path.c_str(), "rb");
#endif

    if (!fp)
    {
        std::cerr << "[AU::Json] Cannot open '" << path << "'\n";
        return false;
    }

    char buf[8192];
    rapidjson::FileReadStream is(fp, buf, sizeof(buf));
    out.ParseStream(is);
    fclose(fp);

    if (out.HasParseError())
    {
        std::cerr << "[AU::Json] Parse error in '" << path << "'\n";
        return false;
    }

    return true;
}

bool Json::SaveFile(const std::string& path, const rapidjson::Document& doc)
{
    FILE* fp = nullptr;
#ifdef _MSC_VER
    fopen_s(&fp, path.c_str(), "wb");
#else
    fp = fopen(path.c_str(), "wb");
#endif

    if (!fp)
    {
        std::cerr << "[AU::Json] Cannot write '" << path << "'\n";
        return false;
    }

    char buf[8192];
    rapidjson::FileWriteStream os(fp, buf, sizeof(buf));
    rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);
    doc.Accept(writer);
    fclose(fp);

    return true;
}

} // namespace AU
