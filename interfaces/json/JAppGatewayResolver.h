#pragma once

#include "../IAppGateway.h"

#include <core/JSON.h>

// IMPORTANT:
// This file is a lightweight, repo-local replacement for the generated Thunder
// JsonGenerator output. It is intentionally self-contained and avoids including
// <plugins/JSONRPC.h> (which drags in COM/Messaging/WebSocket headers that are
// incompatible with the vendored Core in this workspace).
//
// The AppGateway plugin calls:
//   Exchange::JAppGatewayResolver::Register(*this, mAppGateway);
// where `*this` is a PluginHost::JSONRPC instance. To keep this header decoupled
// from the full JSONRPC header stack, we only require the minimal interface
// expected by that call site: an object exposing Register/Unregister methods
// compatible with PluginHost::JSONRPC's API shape.
//
// We therefore template the register functions on the module type, so the
// concrete JSONRPC class is only required in the translation unit including
// <plugins/JSONRPC.h> (AppGateway.h already does).

namespace WPEFramework {
namespace Exchange {

namespace JAppGatewayResolver {

namespace Version {
constexpr uint8_t Major = 1;
constexpr uint8_t Minor = 0;
constexpr uint8_t Patch = 0;
} // namespace Version

namespace detail {

class ContextData final : public Core::JSON::Container {
public:
    ContextData()
        : Core::JSON::Container()
        , RequestId(0)
        , ConnectionId(0)
        , AppId()
    {
        Add(_T("requestId"), &RequestId);
        Add(_T("connectionId"), &ConnectionId);
        Add(_T("appId"), &AppId);
    }

    Core::JSON::DecUInt32 RequestId;
    Core::JSON::DecUInt32 ConnectionId;
    Core::JSON::String AppId;
};

class ResolveRequestData final : public Core::JSON::Container {
public:
    ResolveRequestData()
        : Core::JSON::Container()
        , Context()
        , RequestId(0)
        , ConnectionId(0)
        , AppId()
        , Origin()
        , Method()
        , Params()
    {
        Add(_T("context"), &Context);

        Add(_T("requestId"), &RequestId);
        Add(_T("connectionId"), &ConnectionId);
        Add(_T("appId"), &AppId);

        Add(_T("origin"), &Origin);
        Add(_T("method"), &Method);

        Add(_T("params"), &Params);
    }

    ContextData Context;

    Core::JSON::DecUInt32 RequestId;
    Core::JSON::DecUInt32 ConnectionId;
    Core::JSON::String AppId;

    Core::JSON::String Origin;
    Core::JSON::String Method;
    Core::JSON::Variant Params;
};

static bool KeyPresent(const string& json, const char* keyWithQuotes)
{
    return (json.find(keyWithQuotes) != string::npos);
}

static string TrimAsciiWhitespace(string s)
{
    const auto isSpace = [](const char c) -> bool {
        return (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r');
    };

    while (!s.empty() && isSpace(s.front())) {
        s.erase(s.begin());
    }
    while (!s.empty() && isSpace(s.back())) {
        s.pop_back();
    }
    return s;
}

static string NormalizeJsonStringValue(const string& input)
{
    string out = TrimAsciiWhitespace(input);

    // Unescape occurrences of \" (repeat until stable, also handles \\\" etc)
    for (;;) {
        const std::size_t pos = out.find("\\\"");
        if (pos == string::npos) {
            break;
        }
        out.replace(pos, 2, "\"");
    }

    out = TrimAsciiWhitespace(out);

    if (out.size() >= 2 && out.front() == '"' && out.back() == '"') {
        out = out.substr(1, out.size() - 2);
    }

    return TrimAsciiWhitespace(out);
}

static bool ExtractContextFields(const ResolveRequestData& req,
                                 const string& rawJson,
                                 uint32_t& requestId,
                                 uint32_t& connectionId,
                                 string& appId)
{
    const bool contextMentioned =
        req.Context.RequestId.IsSet() || req.Context.ConnectionId.IsSet() || req.Context.AppId.IsSet() ||
        KeyPresent(rawJson, "\"context\"");

    auto requiredKeysPresent = [&rawJson]() -> bool {
        return KeyPresent(rawJson, "\"requestId\"") &&
               KeyPresent(rawJson, "\"connectionId\"") &&
               KeyPresent(rawJson, "\"appId\"");
    };

    if (!requiredKeysPresent()) {
        return false;
    }

    if (contextMentioned) {
        requestId = req.Context.RequestId.Value();
        connectionId = req.Context.ConnectionId.Value();
        appId = NormalizeJsonStringValue(req.Context.AppId.Value());
        if (!appId.empty()) {
            return true;
        }
    }

    requestId = req.RequestId.Value();
    connectionId = req.ConnectionId.Value();
    appId = NormalizeJsonStringValue(req.AppId.Value());
    return !appId.empty();
}

static string NormalizeParamsToJsonText(const ResolveRequestData& req)
{
    if (!req.Params.IsSet() || req.Params.IsNull()) {
        return _T("{}");
    }

    if (req.Params.Content() == Core::JSON::Variant::type::STRING) {
        string raw = NormalizeJsonStringValue(req.Params.Value());
        if (raw.empty()) {
            return _T("{}");
        }
        return raw;
    }

    string out = TrimAsciiWhitespace(req.Params.Value());
    if (out.empty()) {
        return _T("{}");
    }
    if (out.size() >= 2 && out.front() == '"' && out.back() == '"') {
        out = NormalizeJsonStringValue(out);
        if (out.empty()) {
            return _T("{}");
        }
    }
    return out;
}

} // namespace detail

PUSH_WARNING(DISABLE_WARNING_UNUSED_FUNCTIONS)

// PUBLIC_INTERFACE
template <typename JSONRPCModuleT>
inline void Register(JSONRPCModuleT& module, IAppGatewayResolver* impl)
{
    /** Register the minimal JSON-RPC method bindings for AppGatewayResolver ("resolve"). */
    ASSERT(impl != nullptr);

    module.Register(_T("resolve"),
        [impl](const Core::JSONRPC::Context& /*ctx*/,
               const string& /*designator*/,
               const string& parameters,
               string& response) -> uint32_t {
            detail::ResolveRequestData req;
            Core::OptionalType<Core::JSON::Error> error;

            if (req.FromString(parameters, error) == false || error.IsSet() == true) {
                response.clear();
                return Core::ERROR_BAD_REQUEST;
            }

            const string origin = detail::NormalizeJsonStringValue(req.Origin.Value());
            const string method = detail::NormalizeJsonStringValue(req.Method.Value());

            if (!req.Origin.IsSet() || origin.empty()) {
                response.clear();
                return Core::ERROR_BAD_REQUEST;
            }
            if (!req.Method.IsSet() || method.empty()) {
                response.clear();
                return Core::ERROR_BAD_REQUEST;
            }

            uint32_t requestId = 0;
            uint32_t connectionId = 0;
            string appId;

            if (!detail::ExtractContextFields(req, parameters, requestId, connectionId, appId)) {
                response.clear();
                return Core::ERROR_BAD_REQUEST;
            }

            const string paramsJson = detail::NormalizeParamsToJsonText(req);

            GatewayContext gw;
            gw.requestId = requestId;
            gw.connectionId = connectionId;
            gw.appId = appId;

            string result;
            const uint32_t rc = impl->Resolve(gw, origin, method, paramsJson, result);
            response = result;
            return rc;
        });
}

// PUBLIC_INTERFACE
template <typename JSONRPCModuleT>
inline void Unregister(JSONRPCModuleT& module)
{
    /** Unregister the JSON-RPC method bindings for AppGatewayResolver ("resolve"). */
    module.Unregister(_T("resolve"));
}

POP_WARNING()

} // namespace JAppGatewayResolver

} // namespace Exchange
} // namespace WPEFramework
