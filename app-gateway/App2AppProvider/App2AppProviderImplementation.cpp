#include "App2AppProviderImplementation.h"

#include <random>
#include <sstream>
#include <iomanip>

namespace WPEFramework {
namespace Plugin {

namespace {
    // Helper set error
    inline void SetError(Exchange::IApp2AppProvider::Error& error, int code, const string& message) {
        error.code = code;
        error.message = message;
    }
}

App2AppProviderImplementation::App2AppProviderImplementation(Exchange::IAppGateway* gateway)
    : _refCount(1)
    , _lock()
    , _capabilityToProvider()
    , _capsByConnection()
    , _correlations()
    , _gateway(gateway) {
    ASSERT(_gateway != nullptr);
    LOGTRACE("App2AppProviderImplementation constructed");
    if (_gateway != nullptr) {
        _gateway->AddRef();
    }
}

App2AppProviderImplementation::~App2AppProviderImplementation() {
    LOGTRACE("App2AppProviderImplementation destructed");
    if (_gateway != nullptr) {
        _gateway->Release();
        _gateway = nullptr;
    }
}

// PUBLIC_INTERFACE
uint32_t App2AppProviderImplementation::AddRef() const {
    return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
}

// PUBLIC_INTERFACE
uint32_t App2AppProviderImplementation::Release() const {
    uint32_t count = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
    if (count == 0) {
        delete this;
    }
    return count;
}

// PUBLIC_INTERFACE
void* App2AppProviderImplementation::QueryInterface(const uint32_t id) {
    void* result = nullptr;
    if (id == Exchange::IApp2AppProvider::ID) {
        result = static_cast<Exchange::IApp2AppProvider*>(this);
    } else if (id == Core::IUnknown::ID) {
        result = static_cast<Core::IUnknown*>(this);
    }

    if (result != nullptr) {
        AddRef();
    }
    return result;
}

bool App2AppProviderImplementation::ParseConnectionId(const string& in, uint32_t& out) const {
    if (in.empty()) {
        return false;
    }
    // Accept decimal or hex prefixed with 0x
    try {
        size_t idx = 0;
        int base = 10;
        if ((in.size() > 2) && (in[0] == '0') && (in[1] == 'x' || in[1] == 'X')) {
            base = 16;
        }
        unsigned long val = std::stoul(in, &idx, base);
        if (idx != in.size()) {
            return false;
        }
        out = static_cast<uint32_t>(val);
        return true;
    } catch (...) {
        return false;
    }
}

string App2AppProviderImplementation::GenerateCorrelationId() const {
    // Simple UUID-like hex string generation
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;

    uint64_t a = dis(gen);
    uint64_t b = dis(gen);
    std::ostringstream oss;
    oss << std::hex << std::nouppercase << std::setfill('0')
        << std::setw(16) << a
        << std::setw(16) << b;
    return oss.str();
}

string App2AppProviderImplementation::ExtractCorrelationIdFromPayload(const string& payload) {
    // Minimal extraction to avoid full JSON dependency: look for "correlationId":"<value>"
    const char* key = "\"correlationId\"";
    size_t p = payload.find(key);
    if (p == string::npos) {
        return string();
    }
    p = payload.find(':', p);
    if (p == string::npos) {
        return string();
    }
    // Skip whitespace and quotes
    while (p < payload.size() && (payload[p] == ':' || payload[p] == ' ' || payload[p] == '\t' || payload[p] == '\r' || payload[p] == '\n' || payload[p] == '\"')) {
        ++p;
    }
    // Capture until next quote or comma/brace
    size_t start = p;
    while (p < payload.size() && payload[p] != '\"' && payload[p] != ',' && payload[p] != '}' && payload[p] != '\n' && payload[p] != '\r') {
        ++p;
    }
    if (p <= start) {
        return string();
    }
    return payload.substr(start, p - start);
}

// PUBLIC_INTERFACE
Core::hresult App2AppProviderImplementation::RegisterProvider(const Context& context,
                                                              bool reg,
                                                              const string& capability,
                                                              Error& error) {
    LOGTRACE("RegisterProvider enter: capability=%s appId=%s conn=%s req=%d",
             capability.c_str(), context.appId.c_str(), context.connectionId.c_str(), context.requestId);

    if (capability.empty()) {
        SetError(error, Core::ERROR_BAD_REQUEST, "capability empty");
        LOGERR("RegisterProvider invalid: empty capability");
        return Core::ERROR_BAD_REQUEST;
    }

    uint32_t connId = 0;
    if (!ParseConnectionId(context.connectionId, connId)) {
        SetError(error, Core::ERROR_BAD_REQUEST, "invalid connectionId");
        LOGERR("RegisterProvider invalid connectionId: '%s'", context.connectionId.c_str());
        return Core::ERROR_BAD_REQUEST;
    }

    Core::hresult rc = Core::ERROR_NONE;

    {
        std::lock_guard<std::mutex> guard(_lock);

        if (reg) {
            ProviderEntry entry;
            entry.appId = context.appId;
            entry.connectionId = connId;
            entry.registeredAt = Core::Time::Now();

            _capabilityToProvider[capability] = entry;
            _capsByConnection[connId].insert(capability);

            LOGINFO("Provider registered: capability=%s appId=%s connId=%u",
                    capability.c_str(), entry.appId.c_str(), entry.connectionId);
        } else {
            auto it = _capabilityToProvider.find(capability);
            if (it == _capabilityToProvider.end()) {
                SetError(error, Core::ERROR_UNKNOWN_KEY, "capability not registered");
                LOGWARN("Unregister: capability not found: %s", capability.c_str());
                rc = Core::ERROR_UNKNOWN_KEY;
            } else if (it->second.connectionId != connId) {
                SetError(error, Core::ERROR_GENERAL, "unregister not allowed (ownership mismatch)");
                LOGERR("Unregister denied: capability=%s ownerConnId=%u reqConnId=%u",
                       capability.c_str(), it->second.connectionId, connId);
                rc = Core::ERROR_GENERAL;
            } else {
                // Remove from both maps
                _capabilityToProvider.erase(it);
                auto& caps = _capsByConnection[connId];
                caps.erase(capability);
                if (caps.empty()) {
                    _capsByConnection.erase(connId);
                }
                LOGINFO("Provider unregistered: capability=%s connId=%u", capability.c_str(), connId);
            }
        }
    }

    LOGTRACE("RegisterProvider exit: hr=%d", rc);
    return rc;
}

// PUBLIC_INTERFACE
Core::hresult App2AppProviderImplementation::InvokeProvider(const Context& context,
                                                            const string& capability,
                                                            Error& error) {
    LOGTRACE("InvokeProvider enter: capability=%s appId=%s conn=%s req=%d",
             capability.c_str(), context.appId.c_str(), context.connectionId.c_str(), context.requestId);

    if (_gateway == nullptr) {
        SetError(error, Core::ERROR_UNAVAILABLE, "AppGateway unavailable");
        LOGERR("InvokeProvider failed: IAppGateway unavailable");
        return Core::ERROR_UNAVAILABLE;
    }

    if (capability.empty()) {
        SetError(error, Core::ERROR_BAD_REQUEST, "capability empty");
        LOGERR("InvokeProvider invalid: empty capability");
        return Core::ERROR_BAD_REQUEST;
    }

    uint32_t connId = 0;
    if (!ParseConnectionId(context.connectionId, connId)) {
        SetError(error, Core::ERROR_BAD_REQUEST, "invalid connectionId");
        LOGERR("InvokeProvider invalid connectionId: '%s'", context.connectionId.c_str());
        return Core::ERROR_BAD_REQUEST;
    }

    // Ensure provider exists
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto it = _capabilityToProvider.find(capability);
        if (it == _capabilityToProvider.end()) {
            SetError(error, Core::ERROR_UNKNOWN_KEY, "no provider for capability");
            LOGWARN("InvokeProvider: no provider registered for %s", capability.c_str());
            return Core::ERROR_UNKNOWN_KEY;
        }
    }

    // Create and store correlation with consumer context
    const string correlationId = GenerateCorrelationId();
    {
        std::lock_guard<std::mutex> guard(_lock);
        ConsumerContext cc;
        cc.requestId = static_cast<uint32_t>(context.requestId);
        cc.connectionId = connId;
        cc.appId = context.appId;
        cc.capability = capability;
        cc.createdAt = Core::Time::Now();

        _correlations[correlationId] = std::move(cc);
    }

    // Note: The actual routing to the provider is AppGateway's responsibility.
    // The correlationId must be injected by AppGateway into the outbound message
    // to the provider. Since the interface does not return correlationId, we log it.
    LOGINFO("InvokeProvider: correlationId=%s capability=%s consumerConnId=%u",
            correlationId.c_str(), capability.c_str(), connId);

    LOGTRACE("InvokeProvider exit: hr=%d", Core::ERROR_NONE);
    return Core::ERROR_NONE;
}

// PUBLIC_INTERFACE
Core::hresult App2AppProviderImplementation::HandleProviderResponse(const string& payload,
                                                                    const string& capability,
                                                                    Error& error) {
    LOGTRACE("HandleProviderResponse enter: capability=%s", capability.c_str());

    if (_gateway == nullptr) {
        SetError(error, Core::ERROR_UNAVAILABLE, "AppGateway unavailable");
        LOGERR("HandleProviderResponse failed: IAppGateway unavailable");
        return Core::ERROR_UNAVAILABLE;
    }

    const string correlationId = ExtractCorrelationIdFromPayload(payload);
    if (correlationId.empty()) {
        SetError(error, Core::ERROR_BAD_REQUEST, "missing correlationId");
        LOGERR("HandleProviderResponse: missing correlationId in payload");
        return Core::ERROR_BAD_REQUEST;
    }

    ConsumerContext ctxCopy;
    bool found = false;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto it = _correlations.find(correlationId);
        if (it != _correlations.end()) {
            ctxCopy = it->second;
            _correlations.erase(it);
            found = true;
        }
    }

    if (!found) {
        SetError(error, Core::ERROR_UNKNOWN_KEY, "unknown correlationId");
        LOGERR("HandleProviderResponse: unknown correlationId=%s", correlationId.c_str());
        return Core::ERROR_UNKNOWN_KEY;
    }

    // Build gateway context and respond
    Exchange::IAppGateway::Context gwCtx;
    gwCtx.requestId = static_cast<int>(ctxCopy.requestId);
    gwCtx.connectionId = ctxCopy.connectionId;
    gwCtx.appId = ctxCopy.appId;

    Core::hresult rc = _gateway->Respond(gwCtx, payload);
    if (rc != Core::ERROR_NONE) {
        SetError(error, rc, "gateway respond failed");
        LOGERR("HandleProviderResponse: IAppGateway::Respond failed rc=%d", rc);
        return rc;
    }

    LOGINFO("HandleProviderResponse: delivered to consumer (connId=%u) for capability=%s",
            ctxCopy.connectionId, capability.c_str());
    LOGTRACE("HandleProviderResponse exit: hr=%d", Core::ERROR_NONE);
    return Core::ERROR_NONE;
}

// PUBLIC_INTERFACE
Core::hresult App2AppProviderImplementation::HandleProviderError(const string& payload,
                                                                 const string& capability,
                                                                 Error& error) {
    LOGTRACE("HandleProviderError enter: capability=%s", capability.c_str());

    // For now, we treat error path identically to success, just forward payload as-is.
    return HandleProviderResponse(payload, capability, error);
}

// PUBLIC_INTERFACE
void App2AppProviderImplementation::CleanupByConnection(const uint32_t connectionId) {
    LOGTRACE("CleanupByConnection enter: connId=%u", connectionId);

    std::lock_guard<std::mutex> guard(_lock);

    // Remove provider registrations bound to this connection
    auto capsIt = _capsByConnection.find(connectionId);
    if (capsIt != _capsByConnection.end()) {
        for (const auto& cap : capsIt->second) {
            _capabilityToProvider.erase(cap);
            LOGINFO("CleanupByConnection: removed provider for capability=%s", cap.c_str());
        }
        _capsByConnection.erase(capsIt);
    }

    // Remove pending correlations bound to this connection
    for (auto it = _correlations.begin(); it != _correlations.end();) {
        if (it->second.connectionId == connectionId) {
            LOGINFO("CleanupByConnection: removed correlationId=%s", it->first.c_str());
            it = _correlations.erase(it);
        } else {
            ++it;
        }
    }

    LOGTRACE("CleanupByConnection exit");
}

} // namespace Plugin
} // namespace WPEFramework
