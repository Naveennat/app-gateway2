#include <iostream>
#include <string>

#include <core/core.h>
#include <plugins/IDispatcher.h>

#include "../AppGateway.h"
#include "ServiceMock.h"

// Context conversion utilities live under app-gateway2/Supporting_Files
#include <ContextUtils.h>

using WPEFramework::Core::ERROR_BAD_REQUEST;
using WPEFramework::Core::ERROR_NONE;
using WPEFramework::Core::ERROR_NOT_SUPPORTED;
using WPEFramework::Core::ERROR_PRIVILIGED_REQUEST;
using WPEFramework::Core::ERROR_UNAVAILABLE;
using WPEFramework::Core::ERROR_UNKNOWN_METHOD;
using WPEFramework::Plugin::AppGateway;
using WPEFramework::PluginHost::IDispatcher;
using WPEFramework::PluginHost::IPlugin;

// Prototypes for additional l0test cases implemented in AppGateway_Init_DeinitTests.cpp
extern uint32_t Test_Initialize_WithValidConfig_Succeeds();
extern uint32_t Test_Initialize_Twice_Idempotent();
extern uint32_t Test_Deinitialize_Twice_NoCrash();
extern uint32_t Test_JsonRpc_Registration_And_Unregistration();

namespace {

/*
 * TEST-ONLY BOOTSTRAP
 * -------------------
 * These l0tests run the AppGateway plugin in-proc (without the real Thunder host).
 * Some Thunder/WPEFramework code paths (e.g. JSON-RPC resolving) assume that the
 * global Core::WorkerPool singleton is available.
 *
 * In production, Thunder initializes this for the process. For the l0test harness,
 * we explicitly create, run, and tear down the WorkerPool for the lifetime of the
 * test executable, to avoid aborts and to ensure clean shutdown.
 */
class TestOnlyDispatcher final : public WPEFramework::Core::ThreadPool::IDispatcher {
public:
    TestOnlyDispatcher() = default;
    ~TestOnlyDispatcher() override = default;

    TestOnlyDispatcher(const TestOnlyDispatcher&) = delete;
    TestOnlyDispatcher& operator=(const TestOnlyDispatcher&) = delete;

    void Initialize() override
    {
        // No-op for l0tests. The worker threads will call this on start.
    }

    void Deinitialize() override
    {
        // No-op for l0tests. The worker threads will call this on stop.
    }

    void Dispatch(WPEFramework::Core::IDispatch* job) override
    {
        if (job != nullptr) {
            job->Dispatch();
        }
    }
};

/*
 * Optional test-only Core::Messaging sink.
 * Some builds may route tracing/logging via Core::Messaging::IStore.
 * We provide a no-op sink only if none is configured, keeping tests deterministic.
 */
class NullMessageStore final : public WPEFramework::Core::Messaging::IStore {
public:
    NullMessageStore() = default;
    ~NullMessageStore() override = default;

    NullMessageStore(const NullMessageStore&) = delete;
    NullMessageStore& operator=(const NullMessageStore&) = delete;

    bool Default(const WPEFramework::Core::Messaging::Metadata& /*metadata*/) const override
    {
        // Accept defaults; we drop messages anyway.
        return true;
    }

    void Push(const WPEFramework::Core::Messaging::MessageInfo& /*messageInfo*/,
              const WPEFramework::Core::Messaging::IEvent* /*message*/) override
    {
        // Intentionally no-op.
    }
};

class L0TestBootstrap final {
public:
    L0TestBootstrap()
        : _ownsWorkerPool(false)
        , _dispatcher(nullptr)
        , _workerPool(nullptr)
        , _ownsMessageStore(false)
        , _messageStore(nullptr)
    {
        InitializeMessagingIfNeeded();
        InitializeWorkerPoolIfNeeded();
    }

    ~L0TestBootstrap()
    {
        // Ensure teardown happens even if the test process exits early.
        DeinitializeWorkerPoolIfOwned();
        DeinitializeMessagingIfOwned();
    }

    L0TestBootstrap(const L0TestBootstrap&) = delete;
    L0TestBootstrap& operator=(const L0TestBootstrap&) = delete;

private:
    void InitializeWorkerPoolIfNeeded()
    {
        // Core::IWorkerPool is a global singleton used by various Thunder subsystems.
        if (WPEFramework::Core::IWorkerPool::IsAvailable() == false) {
            // Keep sizes modest; we only need basic dispatch for in-proc tests.
            const uint8_t threadCount = 2;
            const uint32_t stackSize = 0;   // use default stack size
            const uint32_t queueSize = 64;  // sufficient for l0 tests

            _dispatcher = new TestOnlyDispatcher();
            _workerPool = new WPEFramework::Core::WorkerPool(threadCount, stackSize, queueSize, _dispatcher);

            WPEFramework::Core::IWorkerPool::Assign(_workerPool);
            _workerPool->Run();

            _ownsWorkerPool = true;
        }
    }

    void DeinitializeWorkerPoolIfOwned()
    {
        if (_ownsWorkerPool == true) {
            // Stop threads and timers cleanly before deleting.
            if (_workerPool != nullptr) {
                _workerPool->Stop();
            }

            // Detach global singleton first, then destroy.
            WPEFramework::Core::IWorkerPool::Assign(nullptr);

            delete _workerPool;
            _workerPool = nullptr;

            delete _dispatcher;
            _dispatcher = nullptr;

            _ownsWorkerPool = false;
        }
    }

    void InitializeMessagingIfNeeded()
    {
        // Only install a test sink if none is configured by the runtime/build.
        if (WPEFramework::Core::Messaging::IStore::Instance() == nullptr) {
            _messageStore = new NullMessageStore();
            WPEFramework::Core::Messaging::IStore::Set(_messageStore);
            _ownsMessageStore = true;
        }
    }

    void DeinitializeMessagingIfOwned()
    {
        if (_ownsMessageStore == true) {
            WPEFramework::Core::Messaging::IStore::Set(nullptr);
            delete _messageStore;
            _messageStore = nullptr;
            _ownsMessageStore = false;
        }
    }

private:
    bool _ownsWorkerPool;
    TestOnlyDispatcher* _dispatcher;
    WPEFramework::Core::WorkerPool* _workerPool;

    bool _ownsMessageStore;
    NullMessageStore* _messageStore;
};

struct TestResult {
    uint32_t failures { 0 };
};

void ExpectTrue(TestResult& tr, const bool condition, const std::string& what)
{
    if (!condition) {
        tr.failures++;
        std::cerr << "FAIL: " << what << std::endl;
    }
}

void ExpectEqU32(TestResult& tr, const uint32_t actual, const uint32_t expected, const std::string& what)
{
    if (actual != expected) {
        tr.failures++;
        std::cerr << "FAIL: " << what << " expected=" << expected << " actual=" << actual << std::endl;
    }
}

void ExpectEqStr(TestResult& tr, const std::string& actual, const std::string& expected, const std::string& what)
{
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

    ~PluginAndService()
    {
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

static std::string ResolveParamsJson(const std::string& method, const std::string& params = "{}")
{
    return std::string("{")
        + "\"requestId\": 1001,"
        + "\"connectionId\": 10,"
        + "\"appId\": \"com.example.test\","
        + "\"origin\": \"org.rdk.AppGateway\","
        + "\"method\": \"" + method + "\","
        + "\"params\": \"" + params + "\""
        + "}";
}

} // namespace

static uint32_t Test_Initialize_Deinitialize_HappyPath()
{
    TestResult tr;
    PluginAndService ps;

    const std::string initResult = ps.plugin->Initialize(ps.service);
    ExpectEqStr(tr, initResult, "", "Initialize() happy path returns empty string");

    auto dispatcher = ps.plugin->QueryInterface<IDispatcher>();
    ExpectTrue(tr, dispatcher != nullptr, "IDispatcher available after Initialize()");
    if (dispatcher != nullptr) {
        dispatcher->Release();
    }

    ps.plugin->Deinitialize(ps.service);
    return tr.failures;
}

static uint32_t Test_JsonRpcResolve_Success()
{
    TestResult tr;
    PluginAndService ps;

    const std::string initResult = ps.plugin->Initialize(ps.service);
    ExpectEqStr(tr, initResult, "", "Initialize() succeeds for JSON-RPC resolve test");

    auto dispatcher = ps.plugin->QueryInterface<IDispatcher>();
    ExpectTrue(tr, dispatcher != nullptr, "IDispatcher available");
    if (dispatcher != nullptr) {
        const std::string paramsJson = ResolveParamsJson("dummy.method", "{}");

        std::string jsonResponse;
        const uint32_t rc = dispatcher->Invoke(nullptr, 0, 0, "", "resolve", paramsJson, jsonResponse);

        ExpectEqU32(tr, rc, ERROR_NONE, "JSON-RPC resolve returns ERROR_NONE");
        ExpectEqStr(tr, jsonResponse, "null", "JSON-RPC resolve response payload matches resolver result");

        dispatcher->Release();
    }

    ps.plugin->Deinitialize(ps.service);
    return tr.failures;
}

static uint32_t Test_DirectResolver_Success()
{
    TestResult tr;
    PluginAndService ps;

    const std::string initResult = ps.plugin->Initialize(ps.service);
    ExpectEqStr(tr, initResult, "", "Initialize() succeeds for direct resolver test");

    auto* resolver = static_cast<WPEFramework::Exchange::IAppGatewayResolver*>(
        ps.plugin->QueryInterface(WPEFramework::Exchange::IAppGatewayResolver::ID));

    ExpectTrue(tr, resolver != nullptr, "Aggregate IAppGatewayResolver is available via QueryInterface(ID)");
    if (resolver != nullptr) {
        WPEFramework::Exchange::GatewayContext ctx;
        ctx.requestId = 42;
        ctx.connectionId = 7;
        ctx.appId = "com.example.test";

        std::string resolution;
        const uint32_t rc = resolver->Resolve(ctx, "org.rdk.AppGateway", "dummy.method", "{}", resolution);

        ExpectEqU32(tr, rc, ERROR_NONE, "Direct Resolve() returns ERROR_NONE");
        ExpectEqStr(tr, resolution, "null", "Direct Resolve() returns 'null'");

        resolver->Release();
    }

    ps.plugin->Deinitialize(ps.service);
    return tr.failures;
}

static uint32_t Test_JsonRpcResolve_Error_NotPermitted()
{
    TestResult tr;
    PluginAndService ps;

    const std::string initResult = ps.plugin->Initialize(ps.service);
    ExpectEqStr(tr, initResult, "", "Initialize() succeeds for NotPermitted test");

    auto dispatcher = ps.plugin->QueryInterface<IDispatcher>();
    ExpectTrue(tr, dispatcher != nullptr, "IDispatcher available");
    if (dispatcher != nullptr) {
        const std::string paramsJson = ResolveParamsJson("l0.notPermitted", "{}");
        std::string jsonResponse;
        const uint32_t rc = dispatcher->Invoke(nullptr, 0, 0, "", "resolve", paramsJson, jsonResponse);

        ExpectEqU32(tr, rc, ERROR_PRIVILIGED_REQUEST, "NotPermitted maps to ERROR_PRIVILIGED_REQUEST");
        dispatcher->Release();
    }

    ps.plugin->Deinitialize(ps.service);
    return tr.failures;
}

static uint32_t Test_JsonRpcResolve_Error_NotSupported()
{
    TestResult tr;
    PluginAndService ps;

    const std::string initResult = ps.plugin->Initialize(ps.service);
    ExpectEqStr(tr, initResult, "", "Initialize() succeeds for NotSupported test");

    auto dispatcher = ps.plugin->QueryInterface<IDispatcher>();
    ExpectTrue(tr, dispatcher != nullptr, "IDispatcher available");
    if (dispatcher != nullptr) {
        const std::string paramsJson = ResolveParamsJson("l0.notSupported", "{}");
        std::string jsonResponse;
        const uint32_t rc = dispatcher->Invoke(nullptr, 0, 0, "", "resolve", paramsJson, jsonResponse);

        ExpectEqU32(tr, rc, ERROR_NOT_SUPPORTED, "NotSupported maps to ERROR_NOT_SUPPORTED");
        dispatcher->Release();
    }

    ps.plugin->Deinitialize(ps.service);
    return tr.failures;
}

static uint32_t Test_JsonRpcResolve_Error_NotAvailable()
{
    TestResult tr;
    PluginAndService ps;

    const std::string initResult = ps.plugin->Initialize(ps.service);
    ExpectEqStr(tr, initResult, "", "Initialize() succeeds for NotAvailable test");

    auto dispatcher = ps.plugin->QueryInterface<IDispatcher>();
    ExpectTrue(tr, dispatcher != nullptr, "IDispatcher available");
    if (dispatcher != nullptr) {
        const std::string paramsJson = ResolveParamsJson("l0.notAvailable", "{}");
        std::string jsonResponse;
        const uint32_t rc = dispatcher->Invoke(nullptr, 0, 0, "", "resolve", paramsJson, jsonResponse);

        ExpectEqU32(tr, rc, ERROR_UNAVAILABLE, "NotAvailable maps to ERROR_UNAVAILABLE");
        dispatcher->Release();
    }

    ps.plugin->Deinitialize(ps.service);
    return tr.failures;
}

static uint32_t Test_JsonRpcResolve_Error_MalformedInput()
{
    TestResult tr;
    PluginAndService ps;

    const std::string initResult = ps.plugin->Initialize(ps.service);
    ExpectEqStr(tr, initResult, "", "Initialize() succeeds for malformed input test");

    auto dispatcher = ps.plugin->QueryInterface<IDispatcher>();
    ExpectTrue(tr, dispatcher != nullptr, "IDispatcher available");
    if (dispatcher != nullptr) {
        const std::string badJson = "{ this is not valid json }";
        std::string jsonResponse;
        const uint32_t rc = dispatcher->Invoke(nullptr, 0, 0, "", "resolve", badJson, jsonResponse);

        ExpectEqU32(tr, rc, ERROR_BAD_REQUEST, "Malformed JSON returns ERROR_BAD_REQUEST");
        dispatcher->Release();
    }

    ps.plugin->Deinitialize(ps.service);
    return tr.failures;
}

static uint32_t Test_JsonRpcResolve_Error_MissingHandler_WhenResolverMissing()
{
    TestResult tr;

    // Provide no resolver (so JAppGatewayResolver::Register is never called).
    // Provide no responder as well to keep Initialize in a deterministic failure state.
    PluginAndService ps(L0Test::ServiceMock::Config(false, false));

    const std::string initResult = ps.plugin->Initialize(ps.service);
    ExpectTrue(tr, !initResult.empty(), "Initialize() fails when resolver/responder are missing (expected)");

    auto dispatcher = ps.plugin->QueryInterface<IDispatcher>();
    ExpectTrue(tr, dispatcher != nullptr, "IDispatcher is still available even if Initialize() failed");
    if (dispatcher != nullptr) {
        const std::string paramsJson = ResolveParamsJson("dummy.method", "{}");
        std::string jsonResponse;
        const uint32_t rc = dispatcher->Invoke(nullptr, 0, 0, "", "resolve", paramsJson, jsonResponse);

        // Because Register() did not happen, "resolve" is not in the dispatcher.
        ExpectEqU32(tr, rc, ERROR_UNKNOWN_METHOD, "Missing resolve handler returns ERROR_UNKNOWN_METHOD");
        dispatcher->Release();
    }

    // Deinitialize still cleans up mService reference acquired by Initialize().
    ps.plugin->Deinitialize(ps.service);
    return tr.failures;
}

static uint32_t Test_ContextUtils_Conversions()
{
    TestResult tr;

    WPEFramework::Exchange::GatewayContext gw;
    gw.requestId = 7;
    gw.connectionId = 11;
    gw.appId = "app.id";

    const std::string origin = "org.rdk.AppGateway";

    auto notif = ContextUtils::ConvertAppGatewayToNotificationContext(gw, origin);
    ExpectEqU32(tr, notif.requestId, gw.requestId, "NotificationContext.requestId matches");
    ExpectEqU32(tr, notif.connectionId, gw.connectionId, "NotificationContext.connectionId matches");
    ExpectEqStr(tr, notif.appId, gw.appId, "NotificationContext.appId matches");
    ExpectEqStr(tr, notif.origin, origin, "NotificationContext.origin matches");

    auto gw2 = ContextUtils::ConvertNotificationToAppGatewayContext(notif);
    ExpectEqU32(tr, gw2.requestId, gw.requestId, "ConvertNotificationToAppGatewayContext.requestId");
    ExpectEqU32(tr, gw2.connectionId, gw.connectionId, "ConvertNotificationToAppGatewayContext.connectionId");
    ExpectEqStr(tr, gw2.appId, gw.appId, "ConvertNotificationToAppGatewayContext.appId");

    auto provider = ContextUtils::ConvertAppGatewayToProviderContext(gw, origin);
    ExpectEqU32(tr, static_cast<uint32_t>(provider.requestId), gw.requestId, "Provider.Context.requestId matches (int in stub)");
    ExpectEqU32(tr, provider.connectionId, gw.connectionId, "Provider.Context.connectionId matches");
    ExpectEqStr(tr, provider.appId, gw.appId, "Provider.Context.appId matches");
    ExpectEqStr(tr, provider.origin, origin, "Provider.Context.origin matches");

    auto gw3 = ContextUtils::ConvertProviderToAppGatewayContext(provider);
    ExpectEqU32(tr, gw3.requestId, gw.requestId, "ConvertProviderToAppGatewayContext.requestId");
    ExpectEqU32(tr, gw3.connectionId, gw.connectionId, "ConvertProviderToAppGatewayContext.connectionId");
    ExpectEqStr(tr, gw3.appId, gw.appId, "ConvertProviderToAppGatewayContext.appId");

    return tr.failures;
}

int main()
{
    // Test-only bootstrap for WorkerPool (and optional Core::Messaging store).
    // This must be constructed before any plugin Initialize()/Invoke() calls.
    L0TestBootstrap bootstrap;

    // Run instructions (from repo root):
    //   export LD_LIBRARY_PATH=$PWD/dependencies/install/lib:$PWD/build/appgatewayl0test/AppGateway:$LD_LIBRARY_PATH
    //   ./build/appgatewayl0test/AppGateway/appgateway_l0test

    struct Case {
        const char* name;
        uint32_t (*fn)();
    };

    const Case cases[] = {
        // New Init/Deinit lifecycle tests
        { "Initialize_WithValidConfig_Succeeds", Test_Initialize_WithValidConfig_Succeeds },
        { "Initialize_Twice_Idempotent", Test_Initialize_Twice_Idempotent },
        { "Deinitialize_Twice_NoCrash", Test_Deinitialize_Twice_NoCrash },
        { "JsonRpc_Registration_And_Unregistration", Test_JsonRpc_Registration_And_Unregistration },

        // Existing tests
        { "Initialize_Deinitialize_HappyPath", Test_Initialize_Deinitialize_HappyPath },
        { "JsonRpcResolve_Success", Test_JsonRpcResolve_Success },
        { "DirectResolver_Success", Test_DirectResolver_Success },
        { "JsonRpcResolve_Error_NotPermitted", Test_JsonRpcResolve_Error_NotPermitted },
        { "JsonRpcResolve_Error_NotSupported", Test_JsonRpcResolve_Error_NotSupported },
        { "JsonRpcResolve_Error_NotAvailable", Test_JsonRpcResolve_Error_NotAvailable },
        { "JsonRpcResolve_Error_MalformedInput", Test_JsonRpcResolve_Error_MalformedInput },
        { "JsonRpcResolve_Error_MissingHandler_WhenResolverMissing", Test_JsonRpcResolve_Error_MissingHandler_WhenResolverMissing },
        { "ContextUtils_Conversions", Test_ContextUtils_Conversions },
    };

    uint32_t failures = 0;

    for (const auto& c : cases) {
        std::cerr << "[ RUN      ] " << c.name << std::endl;
        const uint32_t f = c.fn();
        if (f == 0) {
            std::cerr << "[       OK ] " << c.name << std::endl;
        } else {
            std::cerr << "[  FAILED  ] " << c.name << " failures=" << f << std::endl;
        }
        failures += f;
    }

    if (failures == 0) {
        std::cout << "AppGateway l0test passed." << std::endl;
        return 0;
    }

    std::cerr << "AppGateway l0test total failures: " << failures << std::endl;
    return static_cast<int>(failures);
}
