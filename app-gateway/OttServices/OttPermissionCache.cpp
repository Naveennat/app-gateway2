#include "OttPermissionCache.h"

namespace WPEFramework {
namespace Plugin {

// PUBLIC_INTERFACE
OttPermissionCache& OttPermissionCache::Instance() {
    static OttPermissionCache g_instance;
    return g_instance;
}

// PUBLIC_INTERFACE
std::vector<string> OttPermissionCache::GetPermissions(const string& appId) const {
    std::lock_guard<std::mutex> lock(_admin);
    auto it = _cache.find(appId);
    if (it != _cache.end()) {
        return it->second;
    }
    return std::vector<string>();
}

// PUBLIC_INTERFACE
void OttPermissionCache::UpdateCache(const string& appId, const std::vector<string>& permissions) {
    std::lock_guard<std::mutex> lock(_admin);
    _cache[appId] = permissions;
}

// PUBLIC_INTERFACE
void OttPermissionCache::Invalidate(const string& appId) {
    std::lock_guard<std::mutex> lock(_admin);
    _cache.erase(appId);
}

// PUBLIC_INTERFACE
void OttPermissionCache::Clear() {
    std::lock_guard<std::mutex> lock(_admin);
    _cache.clear();
}

// PUBLIC_INTERFACE
bool OttPermissionCache::Has(const string& appId) const {
    std::lock_guard<std::mutex> lock(_admin);
    return _cache.find(appId) != _cache.end();
}

// PUBLIC_INTERFACE
size_t OttPermissionCache::Size() const {
    std::lock_guard<std::mutex> lock(_admin);
    return _cache.size();
}

} // namespace Plugin
} // namespace WPEFramework
