#pragma once

/*
 * Minimal stub for the Exchange::INetworkManager interface.
 * Only provides a placeholder interface type to satisfy includes in NetworkDelegate.h.
 */

#include <plugins/Module.h>

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL INetworkManager : virtual public Core::IUnknown {
        // Placeholder ID; real value would be provided by Thunder Interfaces.
        enum { ID = 0xFA000007 };
        virtual ~INetworkManager() = default;

        // PUBLIC_INTERFACE
        virtual Core::hresult GetInternetConnectionStatus(bool& connected /* @out */) = 0;
        /** Retrieve basic internet connectivity status. */
    };

} // namespace Exchange
} // namespace WPEFramework
