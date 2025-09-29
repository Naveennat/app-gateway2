#include "App2AppProviderImplementation.h"

#include <sstream>
#include <iomanip>

#include "UtilsLogging.h"

using namespace WPEFramework;
using namespace WPEFramework::Core;
using namespace WPEFramework::Plugin;

namespace {

// Local JSONRPCDirectLink based on patterns used in ResourceManager/RDKShell.
// Supports various Thunder versions; falls back to generic path if macros are not defined.
struct JSONRPCDirectLink {
private:
    uint32_t _id {0};
    std::string _callsign;

#if ((THUNDER_VERSION >= 4) && (THUNDER_VERSION_MINOR == 4))
    PluginHost::ILocalDispatcher* _dispatcher {nullptr};
#else
    PluginHost::IDispatcher* _dispatcher {nullptr};
#endif

    Core::ProxyType<Core::JSONRPC::Message> Message() const {
        return Core::ProxyType<Core::JSONRPC::Message>(PluginHost::IFactories::Instance().JSONRPC());
    }

    template <typename PARAMETERS>
    bool ToMessage(PARAMETERS& parameters, Core::ProxyType<Core::JSONRPC::Message>& message) const {
        return ToMessage((Core::JSON::IElement*)(&parameters), message);
    }

    bool ToMessage(Core::JSON::IElement* parameters, Core::ProxyType<Core::JSONRPC::Message>& message) const {
        if (!parameters->IsSet()) {
            return true;
        }
        string values;
        if (!parameters->ToString(values)) {
            LOGERR("JSONRPCDirectLink: Failed to convert params to string");
            return false;
        }
        if (!values.empty()) {
            message->Parameters = values;
        }
        return true;
    }

    template <typename RESPONSE>
    bool FromMessage(RESPONSE& response, const Core::ProxyType<Core::JSONRPC::Message>& message, bool isResponseString = false) const {
        return FromMessage((Core::JSON::IElement*)(&response), message, isResponseString);
    }

    bool FromMessage(Core::JSON::IElement* response, const Core::ProxyType<Core::JSONRPC::Message>& message, bool isResponseString = false) const {
        Core::OptionalType<Core::JSON::Error> error;
        if (!isResponseString && !response->FromString(message->Result.Value(), error)) {
            if (error.IsSet() == true) {
                LOGERR("JSONRPCDirectLink: Failed to parse response: '%s'", error.Value().Message().c_str());
            } else {
                LOGERR("JSONRPCDirectLink: Failed to parse response (no error provided)");
            }
            return false;
        }
        return true;
    }

public:
    JSONRPCDirectLink(PluginHost::IShell* service, const std::string& callsign)
        : _callsign(callsign) {
        if (service) {
#if ((THUNDER_VERSION >= 4) && (THUNDER_VERSION_MINOR == 4))
            _dispatcher = service->QueryInterfaceByCallsign<PluginHost::ILocalDispatcher>(_callsign);
#else
            _dispatcher = service->QueryInterfaceByCallsign<PluginHost::IDispatcher>(_callsign);
#endif
        }
    }

    JSONRPCDirectLink(PluginHost::IShell* service)
        : JSONRPCDirectLink(service, "Controller") {
    }

    ~JSONRPCDirectLink() {
        if (_dispatcher)
            _dispatcher->Release();
    }

    template <typename PARAMETERS, typename RESPONSE>
    uint32_t Invoke(const uint32_t waitTime, const std::string& token, const string& method, const PARAMETERS& parameters, RESPONSE& response, bool isResponseString=false) {
        if (_dispatcher == nullptr) {
            LOGERR("JSONRPCDirectLink: No JSON RPC dispatcher for %s", _callsign.c_str());
            return Core::ERROR_GENERAL;
        }

        auto message = Message();

        message->JSONRPC = Core::JSONRPC::Message::DefaultVersion;
        message->Id = Core::JSON::DecUInt32(++_id);
        message->Designator = Core::JSON::String(_callsign + ".1." + method);

        ToMessage(parameters, message);

        const uint32_t channelId = ~0;

#if ((THUNDER_VERSION >= 4) && (THUNDER_VERSION_MINOR == 4))
        string output = "";
        uint32_t result = Core::ERROR_BAD_REQUEST;

        if (_dispatcher != nullptr) {
            PluginHost::ILocalDispatcher* localDispatcher = _dispatcher->Local();
            ASSERT(localDispatcher != nullptr);

            if (localDispatcher != nullptr)
                result = _dispatcher->Invoke(channelId, message->Id.Value(), token, message->Designator.Value(), message->Parameters.Value(), output);
        }

        if (message.IsValid() == true) {
            if (result == static_cast<uint32_t>(~0)) {
                message.Release();
            } else if (result == Core::ERROR_NONE) {
                if (output.empty() == true)
                    message->Result.Null(true);
                else
                    message->Result = output;
            } else {
                message->Error.SetError(result);
                if (output.empty() == false) {
                    message->Error.Text = output;
                }
            }
        }

        if (!FromMessage(response, message, isResponseString)) {
            return Core::ERROR_GENERAL;
        }
        return Core::ERROR_NONE;
#elif (THUNDER_VERSION == 2)
        auto resp = _dispatcher->Invoke(token, channelId, *message);
        if (resp->Error.IsSet()) {
            LOGERR("Call failed: %s error: %s", message->Designator.Value().c_str(), resp->Error.Text.Value().c_str());
            return resp->Error.Code;
        }
        if (!FromMessage(response, resp, isResponseString))
            return Core::ERROR_GENERAL;
        return Core::ERROR_NONE;
#else
        Core::JSONRPC::Context context(channelId, message->Id.Value(), token);
        auto resp = _dispatcher->Invoke(context, *message);

#if ((THUNDER_VERSION >= 4) && (THUNDER_VERSION_MINOR >= 2))
        if (resp->Error.IsSet()) {
            LOGERR("Call failed: %s error: %s", message->Designator.Value().c_str(), resp->Error.Text.Value().c_str());
            return resp->Error.Code;
        }
        if (!FromMessage(response, resp, isResponseString))
            return Core::ERROR_GENERAL;
        return Core::ERROR_NONE;
#else
        // Fallback: assume success if no error structure is available
        if (!FromMessage(response, resp, isResponseString))
            return Core::ERROR_GENERAL;
        return Core::ERROR_NONE;
#endif

#endif
    }
};

} // anonymous

namespace App2App {

// ===== ProviderRegistry =====

ProviderRegistry::ProviderRegistry() : _lock(), _byCapability() {
    LOGINFO("ProviderRegistry created");
}

ProviderRegistry::~ProviderRegistry() {
    LOGINFO("ProviderRegistry destroyed");
}

bool ProviderRegistry::Register(const std::string& capability, const ProviderContext& ctx) {
    LOGINFO("ProviderRegistry::Register capability=%s appId=%s", capability.c_str(), ctx.appId.c_str());
    CriticalSection::Lock guard(_lock);
    // Policy: lastWins (replace existing mapping)
    _byCapability[capability] = ctx;
    return true;
}

bool ProviderRegistry::Unregister(const std::string& capability, const std::string& appId) {
    LOGINFO("ProviderRegistry::Unregister capability=%s appId=%s", capability.c_str(), appId.c_str());
    CriticalSection::Lock guard(_lock);

    auto it = _byCapability.find(capability);
    if (it == _byCapability.end()) {
        LOGWARN("Unregister: capability not found");
        return false;
    }
    if (it->second.appId != appId) {
        LOGWARN("Unregister: not owner (owner=%s)", it->second.appId.c_str());
        return false;
    }
    _byCapability.erase(it);
    return true;
}

bool ProviderRegistry::Resolve(const std::string& capability, ProviderContext& out) const {
    CriticalSection::Lock guard(_lock);

    auto it = _byCapability.find(capability);
    if (it == _byCapability.end()) {
        return false;
    }
    out = it->second;
    return true;
}

// ===== CorrelationMap =====

CorrelationMap::CorrelationMap() : _lock(), _pending() {
    LOGINFO("CorrelationMap created");
}

CorrelationMap::~CorrelationMap() {
    LOGINFO("CorrelationMap destroyed");
}

std::string CorrelationMap::GenerateCorrelationId() const {
    // Generate a pseudo-UUID correlation id
    auto now = Time::Now().Ticks();
    std::random_device rd;
    std::mt19937_64 gen(rd());
    auto rnd = static_cast<uint64_t>(gen());

    std::stringstream ss;
    ss << "corr-" << std::hex << now << "-" << rnd;
    return ss.str();
}

std::string CorrelationMap::Create(const ConsumerContext& ctx) {
    CriticalSection::Lock guard(_lock);
    std::string id = GenerateCorrelationId();
    _pending.emplace(id, ctx);
    LOGINFO("CorrelationMap::Create id=%s appId=%s conn=%s req=%u",
            id.c_str(), ctx.appId.c_str(), ctx.connectionId.c_str(), ctx.requestId);
    return id;
}

bool CorrelationMap::Take(const std::string& correlationId, ConsumerContext& out) {
    CriticalSection::Lock guard(_lock);
    auto it = _pending.find(correlationId);
    if (it == _pending.end()) {
        LOGWARN("CorrelationMap::Take id=%s not found", correlationId.c_str());
        return false;
    }
    out = it->second;
    _pending.erase(it);
    return true;
}

bool CorrelationMap::Peek(const std::string& correlationId, ConsumerContext& out) const {
    CriticalSection::Lock guard(_lock);
    auto it = _pending.find(correlationId);
    if (it == _pending.end()) {
        return false;
    }
    out = it->second;
    return true;
}

// ===== GatewayClient =====

GatewayClient::GatewayClient(PluginHost::IShell* service, const std::string& gatewayCallsign, const std::string& token)
    : _service(service)
    , _gatewayCallsign(gatewayCallsign)
    , _token(token) {
    LOGINFO("GatewayClient constructed for callsign=%s", _gatewayCallsign.c_str());
}

GatewayClient::~GatewayClient() {
    LOGINFO("GatewayClient destroyed");
}

void GatewayClient::UpdateToken(const std::string& token) {
    LOGINFO("GatewayClient::UpdateToken");
    _token = token;
}

uint32_t GatewayClient::Respond(const ConsumerContext& ctx, const Core::JSON::JsonObject& payload) {
    LOGINFO("GatewayClient::Respond to consumer appId=%s conn=%s reqId=%u",
            ctx.appId.c_str(), ctx.connectionId.c_str(), ctx.requestId);

    Core::JSON::JsonObject params;
    Core::JSON::JsonObject context;

    context.Set("appId", Core::JSON::String(ctx.appId));
    context.Set("connectionId", Core::JSON::String(ctx.connectionId));
    context.Set("requestId", Core::JSON::DecUInt32(ctx.requestId));

    params.Set("context", context);
    params.Set("payload", payload);

    Core::JSON::JsonObject result; // not used, we only care about status
    JSONRPCDirectLink client(_service, _gatewayCallsign);
    const uint32_t status = client.Invoke<Core::JSON::JsonObject, Core::JSON::JsonObject>(20000, _token, "respond", params, result);
    if (status != Core::ERROR_NONE) {
        LOGERR("GatewayClient::Respond failed status=%u", status);
    }
    return status;
}

} // namespace App2App
