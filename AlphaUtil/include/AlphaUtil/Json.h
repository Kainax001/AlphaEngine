#pragma once
#include <rapidjson/document.h>
#include <string>

namespace AU {

class Json {
public:
    // Load JSON from file. Returns false on error (file not found / parse error).
    static bool LoadFile(const std::string& path, rapidjson::Document& out);

    // Write Document to file with pretty formatting. Returns false on error.
    static bool SaveFile(const std::string& path, const rapidjson::Document& doc);
};

} // namespace AU
