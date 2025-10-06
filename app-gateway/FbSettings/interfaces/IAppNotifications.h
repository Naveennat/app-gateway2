#pragma once

#include <plugins/plugins.h>

namespace WPEFramework {
namespace Exchange {

    // Minimal stub to satisfy compilation for delegates that hold or pass this interface.
    struct EXTERNAL IAppNotifications : virtual public Core::IUnknown {
        enum { ID = 0x0000FB02 }; // Local arbitrary ID.
        virtual ~IAppNotifications() = default;

        // In a full implementation, this might carry a Notify() or Dispatch() method.
        // We deliberately keep it empty as the BaseEventDelegate::Dispatch is a no-op stub.
    };

} // namespace Exchange
} // namespace WPEFramework
