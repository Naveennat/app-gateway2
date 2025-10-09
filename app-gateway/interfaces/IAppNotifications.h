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

        // PUBLIC_INTERFACE
        virtual void Notify(const string& event /* @in */, const string& payload /* @in */) = 0;
        /** Dispatch an application-level event notification to listeners.
         *  @param event Event name (e.g., "TextToSpeech.onEnabled").
         *  @param payload Serialized payload (JSON or plain string) associated with the event.
         */
    };

} // namespace Exchange
} // namespace WPEFramework
