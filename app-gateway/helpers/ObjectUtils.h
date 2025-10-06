#pragma once

#include <string>

namespace ObjectUtils {

    // Creates a very small JSON object string with a boolean field: {"<key>": <true/false>}
    inline std::string CreateBooleanJsonString(const std::string& key, bool value) {
        return std::string("{\"") + key + "\": " + (value ? "true" : "false") + "}";
    }

    // Returns a canonical JSON boolean literal text, "true" or "false"
    inline std::string BoolToJsonString(bool value) {
        return value ? "true" : "false";
    }
}
