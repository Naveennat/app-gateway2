#include <iostream>
#include <string>
#include <typeinfo>

#include <core/core.h>

#include <AppGateway.h>
#include "ServiceMock.h"

using WPEFramework::Core::ERROR_BAD_REQUEST;
using WPEFramework::Core::ERROR_NONE;
using WPEFramework::Core::ERROR_UNAVAILABLE;
using WPEFramework::Exchange::IAppGatewayResponder;
using WPEFramework::Exchange::GatewayContext;
using WPEFramework::Plugin::AppGateway;
using WPEFramework::PluginHost::IPlugin;

namespace {

struct TestResult {
    uint32_t failures { 0 };
};

static void ExpectTrue(TestResult& tr, const bool condition, const std::string& what) {
    if (!condition) {
        tr.failures++;
        std::cerr << "FAIL: " << what << std::endl;
    }
}
static void ExpectEqU32(TestResult& tr, const uint32_t actual, const uint32_t expected, const std::string& what) {
    if (actual != expected) {
        tr.failures++;
        std::cerr << "FAIL: " << what << " expected=" << expected << " actual=" << actual << std::endl;
    }
}
static void ExpectEqStr(TestResult& tr, const std::string& actual, const std::string& expected, const std::string& what) {
    if (actual != expected) {
        tr.failures++;
        std::cerr << "FAIL: " << what << " expected='" << expected << "' actual='" << actual << "'" << std::endl;
    }
}

struct PluginAndService {
    L0Test::ServiceMock* service { nullptr };
    IPlugin* plugin { nullptr };

    explicit PluginAndService(const L0Test::ServiceMock::Config& cfg = {})
        : service(new L0Test::ServiceMock(cfg))
        , plugin(WPEFramework::Core::Service<AppGateway>::Create<IPlugin>())
    {
    }

    ~PluginAndService() {
        if (plugin != nullptr) {
            plugin->Release();
            plugin = nullptr;
        }
        if (service != nullptr) {
            service->Release();
            service = nullptr;
        }
    }
};

 // Simple collector for connection change notifications
class ConnChangeCollector : public IAppGatewayResponder::INotification {
public:
    ConnChangeCollector() = default;
    ~ConnChangeCollector() override = default;

    // Implement Core::IUnknown non-owning semantics for test stack object.
    uint32_t AddRef() const override { return 1; }
    uint32_t Release() const override { return WPEFramework::Core::ERROR_NONE; }
    void* QueryInterface(const uint32_t id) override {
        if (id == IAppGatewayResponder::INotification::ID) {
            return static_cast<IAppGatewayResponder::INotification*>(this);
        }
        return nullptr;
    }

    void OnAppConnectionChanged(const string& appId, const uint32_t connectionId, const bool connected) override {
        lastAppId = appId;
        lastConnectionId = connectionId;
        lastConnected = connected;
        receivedCount++;
    }

    uint32_t receivedCount { 0 };
    string lastAppId;
    uint32_t lastConnectionId { 0 };
    bool lastConnected { false };
};

static GatewayContext MakeContext(uint32_t requestId = 100, uint32_t connectionId = 77, const std::string& appId = "com.example.app") {
    GatewayContext ctx;
    ctx.requestId = requestId;
    ctx.connectionId = connectionId;
    ctx.appId = appId;
    return ctx;
}

} // namespace

// PUBLIC_INTERFACE
uint32_t Test_Responder_Register_Unregister_Notifications() {
    TestResult tr;

    PluginAndService ps;
    const std::string initResult = ps.plugin->Initialize(ps.service);
    ExpectEqStr(tr, initResult, "", "Initialize() succeeds for responder register/unregister test");

    auto* responder = static_cast<IAppGatewayResponder*>(
        ps.plugin->QueryInterface(IAppGatewayResponder::ID));
    ExpectTrue(tr, responder != nullptr, "Responder interface available");

    if (responder != nullptr) {
        ConnChangeCollector collector;
        const uint32_t regRc = responder->Register(&collector);
        ExpectEqU32(tr, regRc, ERROR_NONE, "Register returns ERROR_NONE");

        // Drive a synthetic connection change via the test fake
        auto* fake = dynamic_cast<L0Test::ResponderFake*>(responder);
        ExpectTrue(tr, fake != nullptr, "Responder is ResponderFake in l0test");
        if (fake != nullptr) {
            fake->SimulateAppConnectionChanged("com.example.app", 101, true);
            ExpectEqU32(tr, collector.receivedCount, 1u, "Listener receives simulated connection change after Register");
            ExpectEqStr(tr, collector.lastAppId, "com.example.app", "AppId propagated to listener");
            ExpectEqU32(tr, collector.lastConnectionId, 101u, "ConnectionId propagated to listener");
            ExpectTrue(tr, collector.lastConnected, "Connected=true propagated");
        }

        // Now Unregister and ensure no further notifications are delivered
        const uint32_t unregRc = responder->Unregister(&collector);
        ExpectEqU32(tr, unregRc, ERROR_NONE, "Unregister returns ERROR_NONE");

        if (fake != nullptr) {
            fake->SimulateAppConnectionChanged("com.example.app", 102, false);
            ExpectEqU32(tr, collector.receivedCount, 1u, "No additional notifications after Unregister");
        }

        responder->Release();
    }

    ps.plugin->Deinitialize(ps.service);
    return tr.failures;
}

// PUBLIC_INTERFACE
uint32_t Test_Responder_Respond_Success_And_Unavailable() {
    TestResult tr;

    PluginAndService ps;
    const std::string initResult = ps.plugin->Initialize(ps.service);
    ExpectEqStr(tr, initResult, "", "Initialize() succeeds");

    auto* responder = static_cast<IAppGatewayResponder*>(
        ps.plugin->QueryInterface(IAppGatewayResponder::ID));
    ExpectTrue(tr, responder != nullptr, "Responder interface available");

    if (responder != nullptr) {
        auto* fake = dynamic_cast<L0Test::ResponderFake*>(responder);
        ExpectTrue(tr, fake != nullptr, "Responder is ResponderFake");

        GatewayContext ctx = MakeContext(1, 200, "com.x");
        // Transport enabled (default)
        uint32_t rc = responder->Respond(ctx, "{\"ok\":true}");
        ExpectEqU32(tr, rc, ERROR_NONE, "Respond succeeds when transport is available");

        // Disable transport and expect ERROR_UNAVAILABLE
        if (fake != nullptr) {
            fake->SetTransportEnabled(false);
            rc = responder->Respond(ctx, "{\"ok\":true}");
            ExpectEqU32(tr, rc, ERROR_UNAVAILABLE, "Respond returns ERROR_UNAVAILABLE when transport is disabled");
        }

        responder->Release();
    }

    ps.plugin->Deinitialize(ps.service);
    return tr.failures;
}

// PUBLIC_INTERFACE
uint32_t Test_Responder_Emit_Success_And_Unavailable() {
    TestResult tr;

    PluginAndService ps;
    const std::string initResult = ps.plugin->Initialize(ps.service);
    ExpectEqStr(tr, initResult, "", "Initialize() succeeds");

    auto* responder = static_cast<IAppGatewayResponder*>(
        ps.plugin->QueryInterface(IAppGatewayResponder::ID));
    ExpectTrue(tr, responder != nullptr, "Responder interface available");

    if (responder != nullptr) {
        auto* fake = dynamic_cast<L0Test::ResponderFake*>(responder);
        ExpectTrue(tr, fake != nullptr, "Responder is ResponderFake");

        GatewayContext ctx = MakeContext(3, 201, "com.y");
        uint32_t rc = responder->Emit(ctx, "event.name", "{\"a\":1}");
        ExpectEqU32(tr, rc, ERROR_NONE, "Emit succeeds when transport is available");

        if (fake != nullptr) {
            fake->SetTransportEnabled(false);
            rc = responder->Emit(ctx, "event.name", "{\"a\":1}");
            ExpectEqU32(tr, rc, ERROR_UNAVAILABLE, "Emit returns ERROR_UNAVAILABLE when transport is disabled");
        }

        responder->Release();
    }

    ps.plugin->Deinitialize(ps.service);
    return tr.failures;
}

// PUBLIC_INTERFACE
uint32_t Test_Responder_Request_Success_And_Unavailable() {
    TestResult tr;

    PluginAndService ps;
    const std::string initResult = ps.plugin->Initialize(ps.service);
    ExpectEqStr(tr, initResult, "", "Initialize() succeeds");

    auto* responder = static_cast<IAppGatewayResponder*>(
        ps.plugin->QueryInterface(IAppGatewayResponder::ID));
    ExpectTrue(tr, responder != nullptr, "Responder interface available");

    if (responder != nullptr) {
        auto* fake = dynamic_cast<L0Test::ResponderFake*>(responder);
        ExpectTrue(tr, fake != nullptr, "Responder is ResponderFake");

        uint32_t rc = responder->Request(300 /*connectionId*/, 9 /*id*/, "method.x", "{\"b\":2}");
        ExpectEqU32(tr, rc, ERROR_NONE, "Request succeeds when transport is available");

        if (fake != nullptr) {
            fake->SetTransportEnabled(false);
            rc = responder->Request(300 /*connectionId*/, 10 /*id*/, "method.x", "{\"b\":2}");
            ExpectEqU32(tr, rc, ERROR_UNAVAILABLE, "Request returns ERROR_UNAVAILABLE when transport is disabled");
        }

        responder->Release();
    }

    ps.plugin->Deinitialize(ps.service);
    return tr.failures;
}

// PUBLIC_INTERFACE
uint32_t Test_Responder_GetGatewayConnectionContext_Known_And_Unknown() {
    TestResult tr;

    PluginAndService ps;
    const std::string initResult = ps.plugin->Initialize(ps.service);
    ExpectEqStr(tr, initResult, "", "Initialize() succeeds");

    auto* responder = static_cast<IAppGatewayResponder*>(
        ps.plugin->QueryInterface(IAppGatewayResponder::ID));
    ExpectTrue(tr, responder != nullptr, "Responder interface available");

    if (responder != nullptr) {
        auto* fake = dynamic_cast<L0Test::ResponderFake*>(responder);
        ExpectTrue(tr, fake != nullptr, "Responder is ResponderFake");

        // Pre-populate one context for known/unknown checks
        const uint32_t cid = 410;
        fake->SetConnectionContext(cid, "header.user-agent", "UA/1.0");

        std::string value;
        uint32_t rc = responder->GetGatewayConnectionContext(cid, "header.user-agent", value);
        ExpectEqU32(tr, rc, ERROR_NONE, "GetGatewayConnectionContext returns ERROR_NONE for known key");
        ExpectEqStr(tr, value, "UA/1.0", "Known key value matches");

        value.clear();
        rc = responder->GetGatewayConnectionContext(cid, "missing.key", value);
        ExpectEqU32(tr, rc, ERROR_BAD_REQUEST, "Unknown key returns ERROR_BAD_REQUEST");

        value.clear();
        rc = responder->GetGatewayConnectionContext(999 /*unknown cid*/, "header.user-agent", value);
        ExpectEqU32(tr, rc, ERROR_BAD_REQUEST, "Unknown connection returns ERROR_BAD_REQUEST");

        responder->Release();
    }

    ps.plugin->Deinitialize(ps.service);
    return tr.failures;
}
