#pragma once
/*
 * Minimal JSONRPC shim for the isolated AppGateway build/L0 harness.
 *
 * The full Thunder SDK header <WPEFramework/plugins/JSONRPC.h> pulls in COM/Messaging/WebSocket
 * infrastructure that requires newer Core primitives than the ones vendored in this workspace.
 *
 * This header provides just enough of PluginHost::JSONRPC / PluginHost::IDispatcher for
 * AppGateway to compile and for the L0 harness to register/unregister/invoke "resolve".
 */

#include <core/core.h>
#include <core/JSON.h>
#include <core/JSONRPC.h>

#include <functional>
#include <unordered_map>

namespace WPEFramework {

namespace PluginHost {

/**
 * Minimal dispatcher interface marker used by BEGIN_INTERFACE_MAP / INTERFACE_ENTRY in plugins.
 *
 * In real Thunder this interface exposes Invoke/Exists/etc. For the isolated L0 build we only
 * need a type that participates in the interface map and can act as a base for JSONRPC.
 */
struct IDispatcherShim : virtual public Core::IUnknown {
    enum { ID = 0xFFFFFFFF };
};

/**
 * PUBLIC_INTERFACE
 * Compatibility type name: upstream Thunder uses PluginHost::IDispatcher.
 */
using IDispatcher = IDispatcherShim;

/**
 * Minimal JSONRPC class compatible with the small subset of APIs used by AppGateway:
 *  - Register(method, lambda)
 *  - Unregister(method)
 *  - Method(method)->Exists(method)
 */
class JSONRPC : public IDispatcher {
public:
    JSONRPC() = default;
    virtual ~JSONRPC() = default;

    using InvokeFunction = std::function<uint32_t(const Core::JSONRPC::Context&,
                                                  const string& /*designator*/,
                                                  const string& /*parameters*/,
                                                  string& /*response*/)>;

    // Register a JSON-RPC method.
    template <typename F>
    uint32_t Register(const string& method, F&& fn)
    {
        _handlers[method] = InvokeFunction(std::forward<F>(fn));
        return Core::ERROR_NONE;
    }

    // Unregister a JSON-RPC method.
    uint32_t Unregister(const string& method)
    {
        _handlers.erase(method);
        return Core::ERROR_NONE;
    }

    // Direct invoke helper for tests.
    uint32_t Invoke(const Core::JSONRPC::Context& context,
                    const string& designator,
                    const string& method,
                    const string& parameters,
                    string& response)
    {
        auto it = _handlers.find(method);
        if (it == _handlers.end()) {
            response.clear();
            return Core::ERROR_UNKNOWN_KEY;
        }
        return it->second(context, designator, parameters, response);
    }

    /**
     * Minimal facade providing Exists() used by AppGateway.cpp to guard Unregister().
     * This is intentionally NOT Core::JSONRPC::Handler (Thunder-specific); it's just a
     * tiny helper with a compatible Exists() signature.
     */
    class MethodGuard {
    public:
        explicit MethodGuard(JSONRPC& parent)
            : _parent(parent)
        {
        }

        uint32_t Exists(const string& method) const
        {
            return (_parent._handlers.find(method) != _parent._handlers.end())
                ? Core::ERROR_NONE
                : Core::ERROR_UNKNOWN_KEY;
        }

    private:
        JSONRPC& _parent;
    };

    // Return an object capable of checking whether a method exists.
    MethodGuard* Method(const string& /*method*/)
    {
        return &_methodGuard;
    }

private:
    std::unordered_map<string, InvokeFunction> _handlers;
    MethodGuard _methodGuard{ *this };
};

} // namespace PluginHost

namespace Plugin {

/**
 * Minimal Metadata shim used by AppGateway.cpp for static plugin metadata.
 * It is unused by the L0 harness but required for compilation.
 */
template <typename T>
class Metadata {
public:
    Metadata(uint8_t /*major*/, uint8_t /*minor*/, uint8_t /*patch*/,
             std::initializer_list<const char*> /*a*/ = {},
             std::initializer_list<const char*> /*b*/ = {},
             std::initializer_list<const char*> /*c*/ = {})
    {
    }
};

} // namespace Plugin
} // namespace WPEFramework
