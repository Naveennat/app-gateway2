#pragma once

// Exchange interface for AppGateway used by App2AppProvider to route responses back to consumers.

#include <plugins/Module.h>

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IAppGateway : virtual public Core::IUnknown {
        enum { ID = ID_APP_GATEWAY };

        struct Context {
            int requestId {0};          // Request identifier originating from consumer
            uint32_t connectionId {0};  // Numeric connection identifier
            string appId;               // Consumer app identifier
        };

        virtual ~IAppGateway() = default;

        // Send a payload back to the consumer described by the context.
        virtual Core::hresult Respond(const Context& context, const string& payload) = 0;
    };

} // namespace Exchange
} // namespace WPEFramework
