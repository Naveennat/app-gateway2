#pragma once

#include "Module.h"
#include "ResolutionStore.h"

namespace WPEFramework {
namespace Plugin {

/**
 * Resolver
 * Loads, validates, and overlays resolution files. Provides query API.
 */
class Resolver {
public:
    Resolver() = default;
    ~Resolver() = default;

    // PUBLIC_INTERFACE
    bool LoadPaths(const std::vector<string>& paths, string& error);

    // PUBLIC_INTERFACE
    bool Get(const string& appId, const string& method, const Core::JSON::Object& params, Core::JSON::Object& out) const {
        return _store.Get(appId, method, params, out);
    }

private:
    bool LoadFile(const string& path, string& error);
    ResolutionStore _store;
};

} // namespace Plugin
} // namespace WPEFramework
