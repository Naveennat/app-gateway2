#pragma once

#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <atomic>
#include "Module.h"
#include "IApp2AppProvider.h"
#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

class App2AppProviderImplementation final : public Exchange::IApp2AppProvider {
public:
    App2AppProviderImplementation(const App2AppProviderImplementation&) = delete;
    App2AppProviderImplementation& operator=(const App2AppProviderImplementation&) = delete;

    explicit App2AppProviderImplementation(Exchange::IAppGateway* gateway);
    ~App2AppProviderImplementation() override;

    // PUBLIC_INTERFACE
    uint32_t AddRef() const override;
    // PUBLIC_INTERFACE
    uint32_t Release() const override;
    // PUBLIC_INTERFACE
    void* QueryInterface(const uint32_t id) override;

    // PUBLIC_INTERFACE
    Core::hresult RegisterProvider(const Context& context,
                                   bool reg,
                                   const string& capability,
                                   Error& error) override;

    // PUBLIC_INTERFACE
    Core::hresult InvokeProvider(const Context& context,
                                 const string& capability,
                                 Error& error) override;

    // PUBLIC_INTERFACE
    Core::hresult HandleProviderResponse(const string& payload,
                                         const string& capability,
                                         Error& error) override;

    // PUBLIC_INTERFACE
    Core::hresult HandleProviderError(const string& payload,
                                      const string& capability,
                                      Error& error) override;

    // PUBLIC_INTERFACE
    void CleanupByConnection(const uint32_t connectionId);

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

    std::unordered_map<string, ProviderEntry> _capabilityToProvider;
    std::unordered_map<uint32_t, std::unordered_set<string>> _capsByConnection;
    std::unordered_map<string, ConsumerContext> _correlations;

    Exchange::IAppGateway* _gateway; // retained
};

} // namespace Plugin
} // namespace WPEFramework
