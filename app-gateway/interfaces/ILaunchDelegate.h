#pragma once

/*
 * Local stub for internal LaunchDelegate-related interfaces used by AppGateway.
 * Provides ILaunchDelegate (with Context), IAppGatewayResponderInternal and IAppGatewayAuthenticatorInternal.
 */

#include <plugins/Module.h>
#include "IAppGateway.h"

namespace WPEFramework {
namespace Exchange {

    // PUBLIC_INTERFACE
    struct EXTERNAL ILaunchDelegate : virtual public Core::IUnknown {
        // Placeholder interface ID for COM-RPC; sufficient for compilation.
        enum { ID = 0xFA000009 };
        virtual ~ILaunchDelegate() = default;

        // Execution context passed between components.
        struct Context {
            int requestId {0};
            uint32_t connectionId {0};
            string appId;
        };
    };

    // PUBLIC_INTERFACE
    struct EXTERNAL IAppGatewayResponderInternal : virtual public Core::IUnknown {
        // Placeholder interface ID for COM-RPC; sufficient for compilation.
        enum { ID = 0xFA000004 };
        virtual ~IAppGatewayResponderInternal() = default;

        /** Respond to a request by sending the payload to the appropriate destination. */
        // PUBLIC_INTERFACE
        virtual Core::hresult Respond(const IAppGateway::Context& context /* @in */, const string& payload /* @in */) = 0;
    };

    // PUBLIC_INTERFACE
    struct EXTERNAL IAppGatewayAuthenticatorInternal : virtual public Core::IUnknown {
        // Placeholder interface ID for COM-RPC; sufficient for compilation.
        enum { ID = 0xFA000005 };
        virtual ~IAppGatewayAuthenticatorInternal() = default;

        /** Authenticate a session token and return the associated application identifier. */
        // PUBLIC_INTERFACE
        virtual Core::hresult Authenticate(const string& sessionId /* @in */,
                                           string& appId /* @out */) = 0;
    };

} // namespace Exchange
} // namespace WPEFramework
