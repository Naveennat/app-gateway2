#pragma once
#include <string>

namespace ObjectUtils {
// Minimal placeholder helpers used by implementation sources.
inline std::string ToString(const std::string& s) { return s; }

// The real implementation inspects JSON objects; for isolated compilation, return false.
template <typename JSONOBJECT>
inline bool HasBooleanEntry(const JSONOBJECT&, const char*, bool& out) {
    out = false;
    return false;
}
} // namespace ObjectUtils
