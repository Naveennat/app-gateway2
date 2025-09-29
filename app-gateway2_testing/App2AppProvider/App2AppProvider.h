#pragma once
/**
 * App2AppProvider.h
 * Thunder plugin interface for App2AppProvider.
 *
 * Implements WPEFramework::PluginHost::IPlugin and WPEFramework::PluginHost::JSONRPC
 * and exposes the JSON-RPC methods:
 *  - registerProvider
 *  - invokeProvider
 *  - handleProviderResponse
 *  - handleProviderError
 *
 * The plugin manages provider registration, correlates consumer requests to provider responses,
 * and forwards provider results/errors back to consumers via AppGateway.respond (COMRPC local dispatch).
 *
 * Logging is done using UtilsLogging.h and method entry logging via UtilsJsonRpc.h macros when appropriate.
 */

#include <string>
#include <memory>

#include <core/JSON.h>
#include <core/JSONRPC.h>
#include <core/Sync.h>
#include <core/Time.h>
#include <interfaces/IAuthenticate.h> // For SecurityAgent token
#include <plugins/Module.h>           // Thunder plugin module defs

#include "App2AppProviderImplementation.h"

// Logging helpers and JSON-RPC response macros
#include "UtilsLogging.h"
#include "UtilsJsonRpc.h"

namespace WPEFramework {
namespace Plugin {

/**
 * App2AppProvider Thunder plugin.
 */
class App2AppProvider : public PluginHost::IPlugin, public PluginHost::JSONRPC {
public:
    App2AppProvider(const App2AppProvider&) = delete;
    App2AppProvider& operator=(const App2AppProvider&) = delete;

public:
    App2AppProvider();
    ~App2AppProvider() override;

public:
    // PUBLIC_INTERFACE
    /**
     * Initialize the plugin.
     * - Captures the service pointer.
     * - Retrieves the SecurityAgent token for COMRPC local dispatch.
     * - Constructs internal helper components.
     *
     * @param service IShell pointer provided by WPEFramework.
     * @return empty string on success, or an error message on failure.
     */
    const string Initialize(PluginHost::IShell* service) override;

    // PUBLIC_INTERFACE
    /**
     * Deinitialize the plugin.
     * - Releases resources and clears registries.
     *
     * @param service IShell pointer provided by WPEFramework.
     */
    void Deinitialize(PluginHost::IShell* service) override;

    // PUBLIC_INTERFACE
    /**
     * Return plugin information string (diagnostics).
     */
    string Information() const override;

public:
    // Wrapper endpoints exposed over JSON-RPC

    // PUBLIC_INTERFACE
    /**
     * registerProvider
     * Register or unregister a provider for a given capability.
     *
     * Params:
     *  - context: { requestId:number, connectionId:string, appId:string }
     *  - register: boolean
     *  - capability: string
     *
     * Response:
     *  - { "success": true|false, "message"?: string }
     */
    uint32_t RegisterProviderWrapper(const Core::JSON::JsonObject& parameters, Core::JSON::JsonObject& response);

    // PUBLIC_INTERFACE
    /**
     * invokeProvider
     * Invoke a capability on a registered provider on behalf of a consumer.
     * Returns a correlationId used by the provider to send async response.
     *
     * Params:
     *  - context: { requestId:number, connectionId:string, appId:string }
     *  - capability: string
     *  - payload: object (optional, opaque)
     *
     * Response:
     *  - { "success": true, "correlationId": string } on success
     */
    uint32_t InvokeProviderWrapper(const Core::JSON::JsonObject& parameters, Core::JSON::JsonObject& response);

    // PUBLIC_INTERFACE
    /**
     * handleProviderResponse
     * Provider sends successful result for a prior invocation.
     *
     * Params:
     *  - payload: { correlationId: string, result: object }
     *  - capability: string
     *
     * Response:
     *  - { "success": true|false, "message"?: string }
     */
    uint32_t HandleProviderResponseWrapper(const Core::JSON::JsonObject& parameters, Core::JSON::JsonObject& response);

    // PUBLIC_INTERFACE
    /**
     * handleProviderError
     * Provider sends error result for a prior invocation.
     *
     * Params:
     *  - payload: { correlationId: string, error: { code: number, message: string } }
     *  - capability: string
     *
     * Response:
     *  - { "success": true|false, "message"?: string }
     */
    uint32_t HandleProviderErrorWrapper(const Core::JSON::JsonObject& parameters, Core::JSON::JsonObject& response);

private:
    // Internal helpers
    bool ValidateContext(const Core::JSON::JsonObject& context, std::string& appId, std::string& connectionId, uint32_t& requestId) const;
    bool ValidateStringField(const Core::JSON::JsonObject& object, const std::string& field, std::string& out) const;

private:
    PluginHost::IShell* _service;
    std::unique_ptr<App2App::ProviderRegistry> _providers;
    std::unique_ptr<App2App::CorrelationMap> _correlations;
    std::unique_ptr<App2App::GatewayClient> _gateway;
    std::unique_ptr<App2App::PermissionManager> _perms;
    std::string _gatewayCallsign;
};

} // namespace Plugin
} // namespace WPEFramework
