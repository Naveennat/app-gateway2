#pragma once

// Exchange interface for App2AppProvider.
// This interface is implemented by the App2AppProviderImplementation class.

#include <plugins/Module.h>

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IApp2AppProvider : virtual public Core::IUnknown {
        // Placeholder interface ID; sufficient for compilation
        enum { ID = 0xFA00000B };

        // Context provided by the consumer side when invoking a provider.
        struct Context {
            int requestId {0};          // Request identifier from the consumer
            uint32_t connectionId {0};  // Numeric connection identifier (aligns with AppGateway::Context)
            string appId;               // Consumer app identifier
            string origin;              // Origin callsign (e.g., AppGateway or InternalGateway)
        };

        virtual ~IApp2AppProvider() = default;

        // PUBLIC_INTERFACE
        virtual Core::hresult RegisterProvider(const Context& context,
                                               bool reg,
                                               const string& capability) = 0;
        /** Register or unregister a provider offering a capability. */

        // PUBLIC_INTERFACE
        virtual Core::hresult InvokeProvider(const Context& context,
                                             const string& capability,
                                             const string& params) = 0;
        /** Invoke a provider by capability for the given consumer context. */

        // PUBLIC_INTERFACE
        virtual Core::hresult HandleProviderResponse(const string& capability,
                                                     const string& params) = 0;
        /** Handle a provider's successful response payload routed back to the consumer. */

        // PUBLIC_INTERFACE
        virtual Core::hresult HandleProviderError(const string& capability,
                                                  const string& params) = 0;
        /** Handle a provider's error payload routed back to the consumer. */

        // PUBLIC_INTERFACE
        virtual Core::hresult Cleanup(const uint32_t connectionId,
                                      const string& callsign) = 0;
        /** Cleanup any resources associated with a connection and origin callsign. */
    };

} // namespace Exchange
} // namespace WPEFramework
