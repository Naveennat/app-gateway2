#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

class ConnectionRegistry {
public:
    struct Context {
        string connectionId;
        string appId;
        string permissionGroup;
    };

public:
    ConnectionRegistry() = default;
    ~ConnectionRegistry() = default;

    // PUBLIC_INTERFACE
    bool Register(const Context& ctx) {
        Core::CriticalSection::ScopedLock lock(_adminLock);
        _byId[ctx.connectionId] = ctx;
        return true;
    }

    // PUBLIC_INTERFACE
    bool Unregister(const string& connectionId) {
        Core::CriticalSection::ScopedLock lock(_adminLock);
        return (_byId.erase(connectionId) > 0);
    }

    // PUBLIC_INTERFACE
    bool Get(const string& connectionId, Context& out) const {
        Core::CriticalSection::ScopedLock lock(_adminLock);
        auto it = _byId.find(connectionId);
        if (it == _byId.end()) {
            return false;
        }
        out = it->second;
        return true;
    }

private:
    mutable Core::CriticalSection _adminLock;
    std::unordered_map<string, Context> _byId;
};

} // namespace Plugin
} // namespace WPEFramework
