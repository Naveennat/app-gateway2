#pragma once

#include "../IAppGateway.h"

#include <core/JSON.h>
#include <plugins/JSONRPC.h>

namespace WPEFramework {
namespace Exchange {

/*
 * Compatibility JSON-RPC wrapper for IAppGatewayResolver (extracted upstream snapshot).
 *
 * This repository runs L0 tests against a locally-built AppGateway plugin. Depending on
 * include-path ordering, some builds may still pick headers from the extracted upstream
 * tree. Ensure this header is functional (registers "resolve"), not a no-op.
 */
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

    if (contextMentioned) {
        if (!requiredKeysPresent()) {
            return false;
        }
        requestId = req.Context.RequestId.Value();
        connectionId = req.Context.ConnectionId.Value();
        appId = req.Context.AppId.Value();
        return !appId.empty();
    }

    if (!requiredKeysPresent()) {
        return false;
    }
    requestId = req.RequestId.Value();
    connectionId = req.ConnectionId.Value();
    appId = req.AppId.Value();
    return !appId.empty();
}

static string NormalizeParamsToJsonText(const ResolveRequestData& req)
{
    if (!req.Params.IsSet() || req.Params.IsNull()) {
        return _T("{}");
    }

    if (req.Params.Content() == Core::JSON::Variant::type::STRING) {
        string raw = req.Params.Value();
        if (raw.empty()) {
            return _T("{}");
        }
        return raw;
    }

    const string out = req.Params.Value();
    if (out.empty()) {
        return _T("{}");
    }
    return out;
}

} // namespace detail

PUSH_WARNING(DISABLE_WARNING_UNUSED_FUNCTIONS)

// PUBLIC_INTERFACE
inline void Register(PluginHost::JSONRPC& module, IAppGatewayResolver* impl)
{
    /** Register the minimal JSON-RPC method bindings for AppGatewayResolver ("resolve"). */
    ASSERT(impl != nullptr);

    module.RegisterVersion(_T("JAppGatewayResolver"), Version::Major, Version::Minor, Version::Patch);

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

            if (!req.Origin.IsSet() || req.Origin.Value().empty()) {
                response.clear();
                return Core::ERROR_BAD_REQUEST;
            }
            if (!req.Method.IsSet() || req.Method.Value().empty()) {
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
            const uint32_t rc = impl->Resolve(gw, req.Origin.Value(), req.Method.Value(), paramsJson, result);

            response = result;
            return rc;
        });
}

// PUBLIC_INTERFACE
inline void Unregister(PluginHost::JSONRPC& module)
{
    /** Unregister the JSON-RPC method bindings for AppGatewayResolver ("resolve"). */
    module.Unregister(_T("resolve"));
}

POP_WARNING()

} // namespace JAppGatewayResolver

} // namespace Exchange
} // namespace WPEFramework
