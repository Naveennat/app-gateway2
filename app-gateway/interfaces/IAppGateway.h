#pragma once

/*
 * Local stub for the Exchange::IAppGateway and related types
 * Provides Context alias and IAppGatewayRequestHandler used by AppGateway and FbCommon.
 */

#include <plugins/Module.h>
// Include existing project-local IAppGateway definition
#include "../IAppGateway.h"

namespace WPEFramework {
namespace Exchange {
    // Provide a global alias used throughout the AppGateway implementation
    using Context = IAppGateway::Context;

    // PUBLIC_INTERFACE
    struct EXTERNAL IAppGatewayRequestHandler : virtual public Core::IUnknown {
        // Placeholder interface ID for COM-RPC; sufficient for compilation.
        enum { ID = 0xFA000003 };

        virtual ~IAppGatewayRequestHandler() = default;

        /** Handle a request resolved to a COM-RPC handler.
         *  @param context Request context (connection, requestId, appId).
         *  @param method  Fully-qualified method (e.g., "device.screenResolution").
         *  @param payload JSON payload string for the method.
         *  @param result  Output serialized result string (JSON or primitive).
         *  @return Core::ERROR_NONE on success or appropriate error code.
         */
        // PUBLIC_INTERFACE
        virtual Core::hresult HandleAppGatewayRequest(const Context& context /* @in */,
                                                      const string& method /* @in */,
                                                      const string& payload /* @in */,
                                                      string& result /* @out */) = 0;
    };
} // namespace Exchange
} // namespace WPEFramework
