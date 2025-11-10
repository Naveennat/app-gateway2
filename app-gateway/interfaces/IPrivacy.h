#pragma once

/*
 * Minimal stub for Exchange::IPrivacy to satisfy compilation.
 * Provides the PrivacySettingOutData type and getters used by PrivacyDelegate.
 */

#include <plugins/Module.h>

#ifndef ID_PRIVACY
#define ID_PRIVACY 0xFA100500
#endif

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IPrivacy : virtual public Core::IUnknown {
        enum { ID = ID_PRIVACY };

        // Data structure returned by privacy getters
        struct PrivacySettingOutData {
            bool allowed { false };
        };

        // PUBLIC_INTERFACE
        virtual Core::hresult GetPersonalization(PrivacySettingOutData& /* @out */) = 0;
        /** Retrieve whether personalization (DisplayPersonalizedRecommendations) is allowed. */

        // PUBLIC_INTERFACE
        virtual Core::hresult GetWatchHistory(PrivacySettingOutData& /* @out */) = 0;
        /** Retrieve whether watch history is allowed (Remember/Share). */
    };

} // namespace Exchange
} // namespace WPEFramework
