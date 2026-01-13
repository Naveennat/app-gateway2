#include <iostream>
#include <string>
#include <cstdlib>

// Expose private members for targeted white-box L0 tests.
// This is test-only and does not change production code.
#define private public
#include "AppGatewayResponderImplementation.h"
#undef private

#include "ServiceMock.h"

#include <core/core.h>

using WPEFramework::Core::ERROR_BAD_REQUEST;
using WPEFramework::Core::ERROR_NONE;
using WPEFramework::Exchange::GatewayContext;

namespace {

struct TestResult {
    uint32_t failures { 0 };
};

static void ExpectTrue(TestResult& tr, const bool condition, const std::string& what)
{
    if (!condition) {
        tr.failures++;
        std::cerr << "FAIL: " << what << std::endl;
    }
}

static void ExpectEqU32(TestResult& tr, const uint32_t actual, const uint32_t expected, const std::string& what)
{
    if (actual != expected) {
        tr.failures++;
        std::cerr << "FAIL: " << what << " expected=" << expected << " actual=" << actual << std::endl;
    }
}

static void ExpectEqStr(TestResult& tr, const std::string& actual, const std::string& expected, const std::string& what)
{
    if (actual != expected) {
        tr.failures++;
        std::cerr << "FAIL: " << what << " expected='" << expected << "' actual='" << actual << "'" << std::endl;
    }
}

// Notification collector to verify IAppGatewayResponder::Register/Unregister and callback delivery.
class ConnChangeCollector final : public WPEFramework::Exchange::IAppGatewayResponder::INotification {
public:
    ConnChangeCollector() = default;
    ~ConnChangeCollector() override = default;

    // Non-owning semantics: these tests create the collector on the stack.
    uint32_t AddRef() const override { return 1; }
    uint32_t Release() const override { return WPEFramework::Core::ERROR_NONE; }
    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IAppGatewayResponder::INotification::ID) {
            return static_cast<WPEFramework::Exchange::IAppGatewayResponder::INotification*>(this);
        }
        return nullptr;
    }

    void OnAppConnectionChanged(const std::string& appId, const uint32_t connectionId, const bool connected) override
    {
        lastAppId = appId;
        lastConnectionId = connectionId;
        lastConnected = connected;
        receivedCount++;
    }

    uint32_t receivedCount { 0 };
    std::string lastAppId;
    uint32_t lastConnectionId { 0 };
    bool lastConnected { false };
};

static GatewayContext MakeContext(uint32_t requestId, uint32_t connectionId, const std::string& appId)
{
    GatewayContext ctx;
    ctx.requestId = requestId;
    ctx.connectionId = connectionId;
    ctx.appId = appId;
    return ctx;
}

// Create a ServiceMock whose ConfigLine is websocket-config-compatible so
// AppGatewayResponderImplementation::InitializeWebsocket() will parse it and bind to an ephemeral port.
static L0Test::ServiceMock::Config MakeResponderServiceConfig()
{
    L0Test::ServiceMock::Config cfg(true /*resolver*/, true /*responder*/, true /*transport*/);
    cfg.provideAuthenticator = true;
    cfg.provideAppNotifications = true;

    // IMPORTANT: must contain `"connector"` so InitializeWebsocket() attempts parsing.
    // Port 0 lets the OS choose a free ephemeral port, reducing flakiness across runs.
    cfg.configLineOverride = "{\"connector\":\"127.0.0.1:0\"}";
    return cfg;
}

} // namespace

// PUBLIC_INTERFACE
uint32_t Test_AppGatewayResponderImplementation_Register_Unregister_And_CallbackDelivery()
{
    // Case: Notification/event push via Register/Unregister + OnConnectionStatusChanged.
    TestResult tr;

    WPEFramework::Plugin::AppGatewayResponderImplementation responder;
    ConnChangeCollector collector;

    ExpectEqU32(tr, responder.Register(&collector), ERROR_NONE, "Register returns ERROR_NONE");

    responder.OnConnectionStatusChanged("com.test.app", 111, true);
    ExpectEqU32(tr, collector.receivedCount, 1u, "Registered listener receives connection status change");
    ExpectEqStr(tr, collector.lastAppId, "com.test.app", "Listener received appId");
    ExpectEqU32(tr, collector.lastConnectionId, 111u, "Listener received connectionId");
    ExpectTrue(tr, collector.lastConnected == true, "Listener received connected=true");

    ExpectEqU32(tr, responder.Unregister(&collector), ERROR_NONE, "Unregister returns ERROR_NONE");

    responder.OnConnectionStatusChanged("com.test.app", 111, false);
    ExpectEqU32(tr, collector.receivedCount, 1u, "Unregistered listener does not receive further updates");

    return tr.failures;
}

// PUBLIC_INTERFACE
uint32_t Test_AppGatewayResponderImplementation_GetGatewayConnectionContext_EnvInjection_And_EmptyKey()
{
    // Case: edge handling for empty key and test-only env-var injection path.
    TestResult tr;

    WPEFramework::Plugin::AppGatewayResponderImplementation responder;

    std::string value;

    // Edge: empty key => BAD_REQUEST
    ExpectEqU32(tr,
               responder.GetGatewayConnectionContext(1, "" /*empty key*/, value),
               ERROR_BAD_REQUEST,
               "Empty contextKey returns ERROR_BAD_REQUEST");

    // Env injection happy path:
    setenv("APPGATEWAY_TEST_CONN_ID", "410", 1);
    setenv("APPGATEWAY_TEST_CTX_KEY", "header.user-agent", 1);
    setenv("APPGATEWAY_TEST_CTX_VALUE", "UA/1.0", 1);

    value.clear();
    ExpectEqU32(tr,
               responder.GetGatewayConnectionContext(410, "header.user-agent", value),
               ERROR_NONE,
               "Env injected key returns ERROR_NONE");
    ExpectEqStr(tr, value, "UA/1.0", "Env injected value matches");

    // Mismatch connId => BAD_REQUEST
    value.clear();
    ExpectEqU32(tr,
               responder.GetGatewayConnectionContext(411, "header.user-agent", value),
               ERROR_BAD_REQUEST,
               "Env injected triple mismatched connectionId returns ERROR_BAD_REQUEST");

    // Mismatch key => BAD_REQUEST
    value.clear();
    ExpectEqU32(tr,
               responder.GetGatewayConnectionContext(410, "missing.key", value),
               ERROR_BAD_REQUEST,
               "Env injected triple mismatched key returns ERROR_BAD_REQUEST");

    return tr.failures;
}

// PUBLIC_INTERFACE
uint32_t Test_AppGatewayResponderImplementation_Auth_Dispatch_Disconnect_Flows()
{
    // Cases:
    // 1) Error-path handling: auth handler rejects missing/empty session.
    // 2) Happy-path request handling: appId registry + DispatchWsMsg routes to resolver with correct context.
    // 3) Notification/event push: disconnect handler notifies AppNotifications::Notify(...cleanup...)
    TestResult tr;

    L0Test::ServiceMock::Config cfg = MakeResponderServiceConfig();
    L0Test::ServiceMock service(cfg);

    WPEFramework::Plugin::AppGatewayResponderImplementation responder;
    ExpectEqU32(tr, responder.Configure(&service), ERROR_NONE, "Configure() returns ERROR_NONE");

    // 1) Auth error paths (token/query handling).
    ExpectTrue(tr,
               responder.mWsManager._authHandler(10 /*cid*/, "" /*empty token*/) == false,
               "AuthHandler rejects empty token/query");
    ExpectTrue(tr,
               responder.mWsManager._authHandler(10 /*cid*/, "foo=bar") == false,
               "AuthHandler rejects token without session key");
    ExpectTrue(tr,
               responder.mWsManager._authHandler(10 /*cid*/, "session=") == false,
               "AuthHandler rejects session key with empty value");

    // 1) Auth happy path -> appId recorded for connectionId.
    const bool ok = responder.mWsManager._authHandler(10 /*cid*/, "session=l0.session");
    ExpectTrue(tr, ok == true, "AuthHandler accepts session token");

    std::string appId;
    ExpectTrue(tr,
               responder.mAppIdRegistry.Get(10 /*cid*/, appId) == true,
               "AppIdRegistry has entry after successful auth");
    ExpectEqStr(tr, appId, "com.l0.authenticated", "AppIdRegistry stored authenticated appId");

    // 2) Request handling / routing: DispatchWsMsg uses appId + callsign origin + resolver.
    responder.DispatchWsMsg("dummy.method", "{}" /*params*/, 77 /*requestId*/, 10 /*cid*/);

    auto* resolver = service.GetResolverFake();
    ExpectTrue(tr, resolver != nullptr, "ResolverFake was instantiated by DispatchWsMsg");
    if (resolver != nullptr) {
        ExpectEqU32(tr, resolver->resolveCount, 1u, "ResolverFake Resolve() invoked once");
        ExpectEqStr(tr, resolver->lastOrigin, "org.rdk.AppGateway", "Origin passed to resolver matches callsign");
        ExpectEqStr(tr, resolver->lastMethod, "dummy.method", "Method passed to resolver matches websocket designator");
        ExpectEqStr(tr, resolver->lastParams, "{}", "Params passed to resolver matches request params");
        ExpectEqU32(tr, resolver->lastContext.requestId, 77u, "Context.requestId propagated");
        ExpectEqU32(tr, resolver->lastContext.connectionId, 10u, "Context.connectionId propagated");
        ExpectEqStr(tr, resolver->lastContext.appId, "com.l0.authenticated", "Context.appId propagated");
    }

    // 3) Disconnect handler: removes registry entry and best-effort notifies AppNotifications.
    responder.mWsManager._disconnectHandler(10 /*cid*/);

    appId.clear();
    ExpectTrue(tr,
               responder.mAppIdRegistry.Get(10 /*cid*/, appId) == false,
               "AppIdRegistry entry removed on disconnect");

    auto* appNotifs = service.GetAppNotificationsFake();
    ExpectTrue(tr, appNotifs != nullptr, "AppNotificationsFake created on disconnect");
    if (appNotifs != nullptr) {
        ExpectEqU32(tr, appNotifs->notifyCount, 1u, "Disconnect triggers exactly one AppNotifications::Notify()");
        ExpectEqStr(tr, appNotifs->lastEvent, "appgateway.connection.cleanup", "Disconnect Notify() uses cleanup event name");

        // Payload should contain at least the connectionId. Do not validate full JSON formatting here.
        ExpectTrue(tr,
                   appNotifs->lastPayload.find("\"connectionId\":10") != std::string::npos,
                   "Disconnect Notify() payload includes connectionId");
        ExpectTrue(tr,
                   appNotifs->lastPayload.find("\"origin\":\"org.rdk.AppGateway\"") != std::string::npos,
                   "Disconnect Notify() payload includes origin");
    }

    return tr.failures;
}

// PUBLIC_INTERFACE
uint32_t Test_AppGatewayResponderImplementation_DispatchWsMsg_ResolverMissing_NoCrash()
{
    // Case: Error-path handling: resolver interface missing => should not crash.
    TestResult tr;

    L0Test::ServiceMock::Config cfg = MakeResponderServiceConfig();
    cfg.provideResolver = false; // force resolver unavailable
    L0Test::ServiceMock service(cfg);

    WPEFramework::Plugin::AppGatewayResponderImplementation responder;
    ExpectEqU32(tr, responder.Configure(&service), ERROR_NONE, "Configure() returns ERROR_NONE even when resolver is not available");

    // Pretend the connection has been authenticated (avoid triggering Close() path).
    responder.mAppIdRegistry.Add(12 /*cid*/, "com.test.app");

    // Should log error and return; no crash.
    responder.DispatchWsMsg("dummy.method", "{}", 1 /*requestId*/, 12 /*cid*/);

    // Sanity: registry unchanged
    std::string appId;
    ExpectTrue(tr, responder.mAppIdRegistry.Get(12, appId) == true, "AppIdRegistry still present after DispatchWsMsg error path");

    return tr.failures;
}
