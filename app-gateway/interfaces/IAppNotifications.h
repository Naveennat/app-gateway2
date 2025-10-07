#pragma once

/*
 * Minimal stub for the Exchange::IAppNotifications interface to satisfy build-time includes.
 * This interface is used by SettingsDelegate and friends to acquire and hold a reference
 * to an application-wide notifications service.
 */

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IAppNotifications : virtual public Core::IUnknown {
        // Placeholder interface ID; in a full Thunder environment, this would be defined in Ids.h
        enum { ID = 0xFA000002 };

        virtual ~IAppNotifications() = default;
        // No additional methods are required by current code paths.
    };

} // namespace Exchange
} // namespace WPEFramework
