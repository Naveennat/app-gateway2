#pragma once

// Exchange interface for AppGateway used by App2AppProvider to route responses back to consumers,
// and for AppGateway to expose Resolve/Configure paths.

#include <plugins/Module.h>

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IAppGateway : virtual public Core::IUnknown {
        // Placeholder interface ID; sufficient for compilation in this environment
        enum { ID = 0xFA00000A };

        // Iterator over configuration file paths
        struct IStringIterator : virtual public Core::IUnknown {
            enum { ID = 0xFA000008 }; // placeholder
            virtual ~IStringIterator() = default;

            // PUBLIC_INTERFACE
            virtual bool Next(string& current /* @out */) = 0;
            /** Retrieve the next string if available and assign it to current. */
        };

        struct Context {
            int requestId {0};          // Request identifier originating from consumer
            uint32_t connectionId {0};  // Numeric connection identifier
            string appId;               // Consumer app identifier
        };

        virtual ~IAppGateway() = default;

        // PUBLIC_INTERFACE
        virtual Core::hresult Respond(const Context& context, const string& payload) = 0;
        /** Send a payload back to the consumer described by the context. */

        // PUBLIC_INTERFACE
        virtual Core::hresult Configure(IStringIterator* const &paths) = 0;
        /** Configure AppGateway with resolution config file paths. */

        // PUBLIC_INTERFACE
        virtual Core::hresult Resolve(const Context& context /* @in */,
                                      const string& method /* @in */,
                                      const string& params /* @in */) = 0;
        /** Resolve a Firebolt method and route it per the resolution config. */
    };

} // namespace Exchange
} // namespace WPEFramework
