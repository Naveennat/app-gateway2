#pragma once

/*
 * Minimal stub for Exchange::IFbMetrics to satisfy compilation.
 * Declares IFbMetrics with the methods used by FbMetricsImplementation.
 */

#include <plugins/Module.h>

#ifndef ID_FB_METRICS
#define ID_FB_METRICS 0xFA300200
#endif

namespace WPEFramework {
namespace Exchange {

    // PUBLIC_INTERFACE
    struct EXTERNAL IFbMetrics : virtual public Core::IUnknown {
        enum { ID = ID_FB_METRICS };

        // Context structure passed from callers
        struct Context {
            string appId;
        };

        // PUBLIC_INTERFACE
        virtual Core::hresult Action(const Context& context /* @in */,
                                     const string& category /* @in */,
                                     const string& type /* @in */,
                                     const string& parameters /* @in @opaque */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult AppInfo(const Context& context /* @in */,
                                      const string& build /* @in */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult Error(const Context& context /* @in */,
                                    const string& type /* @in */,
                                    const string& code /* @in */,
                                    const string& description /* @in */,
                                    const bool visible /* @in */,
                                    const string& parameters /* @in @opaque */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult MediaEnded(const Context& context /* @in */,
                                         const string& entityId /* @in */) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult MediaLoadStart(const Context& context /* @in */,
                                             const string& entityId /* @in */) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult MediaPause(const Context& context /* @in */,
                                         const string& entityId /* @in */) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult MediaPlay(const Context& context /* @in */,
                                        const string& entityId /* @in */) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult MediaPlaying(const Context& context /* @in */,
                                           const string& entityId /* @in */) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult MediaProgress(const Context& context /* @in */,
                                            const string& entityId /* @in */,
                                            const string& progress /* @in */) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult MediaRateChange(const Context& context /* @in */,
                                              const string& entityId /* @in */,
                                              const double rate /* @in */) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult MediaRenditionChange(const Context& context /* @in */,
                                                   const string& entityId /* @in */,
                                                   const uint32_t bitrate /* @in */,
                                                   const uint32_t width /* @in */,
                                                   const uint32_t height /* @in */,
                                                   const string& profile /* @in */) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult MediaSeeked(const Context& context /* @in */,
                                          const string& entityId /* @in */,
                                          const string& position /* @in */) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult MediaSeeking(const Context& context /* @in */,
                                           const string& entityId /* @in */,
                                           const string& target /* @in */) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult MediaWaiting(const Context& context /* @in */,
                                           const string& entityId /* @in */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult Page(const Context& context /* @in */,
                                   const string& pageId /* @in */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult StartContent(const Context& context /* @in */,
                                           const string& entityId /* @in */) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult StopContent(const Context& context /* @in */,
                                          const string& entityId /* @in */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult SignIn(const Context& context /* @in */,
                                     const string& entitlements /* @in */) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult SignOut(const Context& context /* @in */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult SetAppUserSessionId(const string& id /* @in */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult SetLifeCycle(const Context& context /* @in */,
                                           const string& newState /* @in */,
                                           const string& previousState /* @in */) = 0;
    };

} // namespace Exchange
} // namespace WPEFramework
