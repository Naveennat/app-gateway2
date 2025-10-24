#pragma once

#include <string>
#include <core/JSON.h>

namespace ObjectUtils {

    // Creates a very small JSON object string with a boolean field: {"<key>": <true/false>}
    inline std::string CreateBooleanJsonString(const std::string& key, bool value) {
        return std::string("{\"") + key + "\": " + (value ? "true" : "false") + "}";
    }

    // Returns a canonical JSON boolean literal text, "true" or "false"
    inline std::string BoolToJsonString(bool value) {
        return value ? "true" : "false";
    }

    // PUBLIC_INTERFACE
    template <typename JSONObjectLike>
    inline bool HasBooleanEntry(JSONObjectLike& obj,
                                const std::string& key,
                                bool& outValue) {
        /**
         * Generic helper that works with Core::JSON::VariantContainer-like objects (and JsonObject aliases)
         * to read a boolean entry by key and place it into outValue. Returns true if present and boolean.
         */
        using WPEFramework::Core::JSON::Variant;
        Variant field = obj[key.c_str()];
        if (field.IsSet() && !field.IsNull() && field.Content() == Variant::type::BOOLEAN) {
            outValue = field.Boolean();
            return true;
        }
        return false;
    }
}
