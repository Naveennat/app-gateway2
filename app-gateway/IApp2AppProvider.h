#pragma once

// Exchange interface for App2AppProvider.
// This interface is implemented by the App2AppProviderImplementation class.

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IApp2AppProvider : virtual public Core::IUnknown {
        enum { ID = ID_APP2APP_PROVIDER };

        // Context provided by the consumer side when invoking a provider.
        struct Context {
            int requestId {0};          // Request identifier from the consumer
            string connectionId;        // Connection identifier (string formatted, e.g. "0x1A" or decimal)
            string appId;               // Consumer app identifier
        };

        // Error structure returned by methods on failure.
        struct Error {
            int code {0};
            string message;
        };

        virtual ~IApp2AppProvider() = default;

        // Register or unregister a provider offering a capability.
        virtual Core::hresult RegisterProvider(const Context& context,
                                               bool reg,
                                               const string& capability,
                                               Error& error) = 0;

        // Invoke a provider by capability for the given consumer context.
        virtual Core::hresult InvokeProvider(const Context& context,
                                             const string& capability,
                                             Error& error) = 0;

        // Handle a provider's successful response payload routed back to the consumer.
        virtual Core::hresult HandleProviderResponse(const string& payload,
                                                     const string& capability,
                                                     Error& error) = 0;

        // Handle a provider's error payload routed back to the consumer.
        virtual Core::hresult HandleProviderError(const string& payload,
                                                  const string& capability,
                                                  Error& error) = 0;
    };

} // namespace Exchange
} // namespace WPEFramework
