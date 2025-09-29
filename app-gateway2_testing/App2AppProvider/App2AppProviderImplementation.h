#pragma once
/**
 * App2AppProviderImplementation.h
 * Internal implementation classes for the App2AppProvider plugin:
 *  - ProviderRegistry: capability -> ProviderContext mapping
 *  - CorrelationMap: correlationId -> ConsumerContext mapping
 *  - GatewayClient: COMRPC local dispatcher to call AppGateway.respond
 *  - PermissionManager: stub for future permission enforcement
 */

#include <unordered_map>
#include <string>
#include <random>

#include <core/JSON.h>
#include <core/Sync.h>
#include <core/Time.h>
#include <core/JSONRPC.h>
#include <plugins/Module.h>
#include <interfaces/IAuthenticate.h>

#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {
class App2AppProvider;
} // namespace Plugin
} // namespace WPEFramework

namespace App2App {

struct ProviderContext {
    std::string appId;
    std::string connectionId;
};

struct ConsumerContext {
    std::string appId;
    std::string connectionId;
    uint32_t requestId {0};
};

class ProviderRegistry {
public:
    ProviderRegistry();
    ~ProviderRegistry();

    // Register provider for capability (policy: lastWins)
    bool Register(const std::string& capability, const ProviderContext& ctx);

    // Unregister provider (only owner appId can unregister)
    bool Unregister(const std::string& capability, const std::string& appId);

    // Resolve capability to provider context
    bool Resolve(const std::string& capability, ProviderContext& out) const;

private:
    mutable WPEFramework::Core::CriticalSection _lock;
    std::unordered_map<std::string, ProviderContext> _byCapability;
};

class CorrelationMap {
public:
    CorrelationMap();
    ~CorrelationMap();

    // Create correlation record and return a new correlationId
    std::string Create(const ConsumerContext& ctx);

    // Take (and remove) a correlation record by id
    bool Take(const std::string& correlationId, ConsumerContext& out);

    // Peek (do not remove)
    bool Peek(const std::string& correlationId, ConsumerContext& out) const;

private:
    std::string GenerateCorrelationId() const;

private:
    mutable WPEFramework::Core::CriticalSection _lock;
    std::unordered_map<std::string, ConsumerContext> _pending;
};

class GatewayClient {
public:
    GatewayClient(WPEFramework::PluginHost::IShell* service, const std::string& gatewayCallsign, const std::string& token);
    ~GatewayClient();

    // PUBLIC_INTERFACE
    /**
     * Respond
     * COMRPC invoke of AppGateway.respond with a consumer context and an opaque payload.
     * Returns Core::ERROR_NONE on success; otherwise an error code from dispatcher.
     */
    uint32_t Respond(const ConsumerContext& ctx, const WPEFramework::Core::JSON::JsonObject& payload);

    void UpdateToken(const std::string& token);

private:
    WPEFramework::PluginHost::IShell* _service;
    std::string _gatewayCallsign;
    std::string _token;
};

class PermissionManager {
public:
    PermissionManager() = default;
    ~PermissionManager() = default;

    // PUBLIC_INTERFACE
    /**
     * IsAllowed
     * Placeholder for future permission enforcement. Always allows in phase 1.
     */
    bool IsAllowed(const std::string& /*appId*/, const std::string& /*requiredGroup*/) const {
        LOGDBG("PermissionManager::IsAllowed: always true (Phase 1)");
        return true;
    }
};

} // namespace App2App
