#pragma once
#include <string>

namespace Utils {

// Minimal query parser: finds "<key>=<value>" in a token string (supports '&' separated pairs).
inline std::string ResolveQuery(const std::string& token, const std::string& key)
{
    const std::string needle = key + "=";
    std::size_t start = token.find(needle);
    if (start == std::string::npos) {
        return std::string();
    }
    start += needle.size();
    std::size_t end = token.find('&', start);
    if (end == std::string::npos) {
        end = token.size();
    }
    return token.substr(start, end - start);
}

// Minimal Thunder controller client shim for isolated builds.
// In full RDK/Thunder environments this returns a JSONRPC link to another plugin.
// For L0/unit builds we only need compilation; callers should handle nullptr.
template <typename IShellT>
inline void* GetThunderControllerClient(IShellT* /*service*/, const std::string& /*callsign*/)
{
    return nullptr;
}

} // namespace Utils
