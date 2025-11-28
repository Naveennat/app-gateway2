#pragma once

// Lightweight string helpers used by delegates

#include <string>
#include <algorithm>

namespace StringUtils {

    inline std::string toLower(const std::string& s) {
        std::string out;
        out.resize(s.size());
        std::transform(s.begin(), s.end(), out.begin(),
                       [](unsigned char c) { return static_cast<char>(::tolower(c)); });
        return out;
    }

    // Returns true if 'haystack' contains 'needle' case-insensitively, starting at position 0
    // We use it to simulate a case-insensitive "prefix" check used in delegates.
    inline bool rfindInsensitive(const std::string& haystack, const std::string& needle) {
        if (needle.size() > haystack.size()) {
            return false;
        }
        const std::string lhs = toLower(haystack.substr(0, needle.size()));
        const std::string rhs = toLower(needle);
        return lhs == rhs;
    }
}
