#pragma once

/*
 * Expanded stub for the Exchange::IAppNotifications interface to satisfy build-time includes.
 * Provides minimal methods used by AppGateway and FbSettings to register for events and cleanup.
 */

#include <plugins/Module.h>

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IAppNotifications : virtual public Core::IUnknown {
        // Placeholder interface ID; in a full Thunder environment, this would be defined in Ids.h
        enum { ID = 0xFA000002 };

        virtual ~IAppNotifications() = default;

        // Basic context used by Subscribe/Cleanup in AppGateway
        struct Context {
            int requestId {0};
            uint32_t connectionId {0};
            string appId;   // Added to support conversions in ContextUtils
            string origin;
        };

        // PUBLIC_INTERFACE
        virtual Core::hresult Subscribe(const Context& context /* @in */,
                                        const bool listen /* @in */,
                                        const string& alias /* @in */,
                                        const string& event /* @in */) = 0;
        /** Register/unregister for an event on a specific alias.
         *  @param context Gateway context identifying connection and origin.
         *  @param listen True to subscribe, false to unsubscribe.
         *  @param alias Callsign alias (e.g., "org.rdk.DisplaySettings").
         *  @param event Fully qualified event name.
         */

        // PUBLIC_INTERFACE
        virtual Core::hresult Cleanup(const uint32_t connectionId /* @in */,
                                      const string& callsign /* @in */) = 0;
        /** Cleanup any notifications associated with a connection and callsign. */

        // PUBLIC_INTERFACE
        virtual void Notify(const string& event /* @in */, const string& payload /* @in */) = 0;
        /** Dispatch an application-level event notification to listeners.
         *  @param event Event name (e.g., "TextToSpeech.onEnabled").
         *  @param payload Serialized payload (JSON or plain string) associated with the event.
         */
    };

    // PUBLIC_INTERFACE
    struct EXTERNAL IAppNotificationHandlerInternal : virtual public Core::IUnknown {
        // Placeholder interface ID
        enum { ID = 0xFA000006 };
        virtual ~IAppNotificationHandlerInternal() = default;

        // PUBLIC_INTERFACE
        virtual Core::hresult HandleAppEventNotifier(const string& event /* @in */, const bool listen /* @in */) = 0;
        /** Internal event notifier interface used by FbSettings to forward app event registrations. */
    };

} // namespace Exchange
} // namespace WPEFramework
