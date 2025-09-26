#pragma once

#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <atomic>
#include "Module.h"
#include "IApp2AppProvider.h"
#include "IAppGateway.h"
#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

class App2AppProviderImplementation final : public Exchange::IApp2AppProvider {
public:
    App2AppProviderImplementation(const App2AppProviderImplementation&) = delete;
    App2AppProviderImplementation& operator=(const App2AppProviderImplementation&) = delete;

    explicit App2AppProviderImplementation(Exchange::IAppGateway* gateway);
    ~App2AppProviderImplementation() override;

    // Core::IUnknown implementation
    // PUBLIC_INTERFACE
    uint32_t AddRef() const override;
    /** Increase reference count on the implementation. */

    // PUBLIC_INTERFACE
    uint32_t Release() const override;
    /** Decrease reference count; deletes at zero. */

    // PUBLIC_INTERFACE
    void* QueryInterface(const uint32_t id) override;
    /** Support Exchange::IApp2AppProvider & Core::IUnknown queries. */

    // Exchange::IApp2AppProvider implementation
    // PUBLIC_INTERFACE
    Core::hresult RegisterProvider(const Context& context,
                                   bool reg,
                                   const string& capability,
                                   Error& error) override;
    /** Register or unregister a provider against a capability. */

    // PUBLIC_INTERFACE
    Core::hresult InvokeProvider(const Context& context,
                                 const string& capability,
                                 Error& error) override;
    /** Track a consumer invocation by storing a correlation context. */

    // PUBLIC_INTERFACE
    Core::hresult HandleProviderResponse(const string& payload,
                                         const string& capability,
                                         Error& error) override;
    /** Provider replied; forward to consumer via IAppGateway::Respond. */

    // PUBLIC_INTERFACE
    Core::hresult HandleProviderError(const string& payload,
                                      const string& capability,
                                      Error& error) override;
    /** Provider error; forward error payload to consumer via IAppGateway::Respond. */

    // PUBLIC_INTERFACE
    void CleanupByConnection(const uint32_t connectionId);
    /** Remove all providers and correlations bound to a connection. */

private:
    bool ParseConnectionId(const string& in, uint32_t& out) const;
    string GenerateCorrelationId() const;
    static string ExtractCorrelationIdFromPayload(const string& payload);

private:
    struct ProviderEntry {
        string appId;
        uint32_t connectionId{0};
        Core::Time registeredAt;
    };

    struct ConsumerContext {
        uint32_t requestId{0};
        uint32_t connectionId{0};
        string appId;
        string capability;
        Core::Time createdAt;
    };

private:
    mutable std::atomic<uint32_t> _refCount;
    mutable std::mutex _lock;

    std::unordered_map<string, ProviderEntry> _capabilityToProvider;           // capability -> provider
    std::unordered_map<uint32_t, std::unordered_set<string>> _capsByConnection; // connectionId -> {capability}
    std::unordered_map<string, ConsumerContext> _correlations;                 // correlationId -> context

    Exchange::IAppGateway* _gateway; // retained while implementation lives
};

} // namespace Plugin
} // namespace WPEFramework
