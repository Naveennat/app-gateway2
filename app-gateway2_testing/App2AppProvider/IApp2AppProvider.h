#pragma once

// Exchange interface for App2AppProvider (testing tree).
// Implemented by App2AppProviderImplementation.

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IApp2AppProvider : virtual public Core::IUnknown {
        enum { ID = ID_APP2APP_PROVIDER };

        // Context provided by the consumer side when invoking a provider.
        struct Context {
            int requestId {0};
            string connectionId;
            string appId;
        };

        // Error structure returned by methods on failure.
        struct Error {
            int code {0};
            string message;
        };

        virtual ~IApp2AppProvider() = default;

        // PUBLIC_INTERFACE
        virtual Core::hresult RegisterProvider(const Context& context,
                                               bool reg,
                                               const string& capability,
                                               Error& error) = 0;
        /** Register or unregister a provider offering a capability. */

        // PUBLIC_INTERFACE
        virtual Core::hresult InvokeProvider(const Context& context,
                                             const string& capability,
                                             Error& error) = 0;
        /** Track a consumer invocation by storing a correlation context. */

        // PUBLIC_INTERFACE
        virtual Core::hresult HandleProviderResponse(const string& payload,
                                                     const string& capability,
                                                     Error& error) = 0;
        /** Provider replied; forward to consumer via AppGateway. */

        // PUBLIC_INTERFACE
        virtual Core::hresult HandleProviderError(const string& payload,
                                                  const string& capability,
                                                  Error& error) = 0;
        /** Provider error; forward to consumer via AppGateway. */
    };

    // Minimal IAppGateway subset used by the provider plugin.
    struct EXTERNAL IAppGateway : virtual public Core::IUnknown {
        enum { ID = ID_APP_GATEWAY };

        struct Context {
            int requestId {0};
            uint32_t connectionId {0};
            string appId;
        };

        virtual ~IAppGateway() = default;

        // PUBLIC_INTERFACE
        virtual Core::hresult Respond(const Context& context,
                                      const string& payload) = 0;
        /** Dispatch a payload to a specific app context over the gateway. */
    };

} // namespace Exchange
} // namespace WPEFramework
