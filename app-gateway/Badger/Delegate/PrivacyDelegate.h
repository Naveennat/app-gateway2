#pragma once

/**
 * PrivacyDelegate
 *
 * Encapsulates interactions with the org.rdk.Privacy plugin (Exchange::IPrivacy)
 * and exposes helpers that serialize results into JSON strings, mirroring patterns
 * used by other delegates (e.g., SystemDelegate, DisplaySettingsDelegate).
 */

#include <memory>
#include <string>

#include <core/JSON.h>
#include <plugins/plugins.h>

#include "UtilsLogging.h"

// COM interface
#include <interfaces/IPrivacy.h>

// Callsign
#ifndef PRIVACY_CALLSIGN
#define PRIVACY_CALLSIGN "org.rdk.Privacy"
#endif

namespace WPEFramework {
    class PrivacyDelegate {
    public:
        // PUBLIC_INTERFACE
        explicit PrivacyDelegate(PluginHost::IShell* shell)
            : _shell(shell) {}

        ~PrivacyDelegate() = default;

        // PUBLIC_INTERFACE
        /**
        * GetPersonalizationAllowed
        * Retrieves personalization setting and returns:
        *   {"enabled": <bool>}
        */
        uint32_t GetPersonalizationAllowed(std::string& personalizationJson) const {
            personalizationJson.clear();

            if (_shell == nullptr) {
                LOGERR("PrivacyDelegate: Shell not initialized");
                return Core::ERROR_UNAVAILABLE;
            }

            auto* p = _shell->QueryInterfaceByCallsign<WPEFramework::Exchange::IPrivacy>(PRIVACY_CALLSIGN);
            if (p == nullptr) {
                LOGWARN("PrivacyDelegate: IPrivacy (%s) not available", PRIVACY_CALLSIGN);
                return Core::ERROR_UNAVAILABLE;
            }

            WPEFramework::Exchange::IPrivacy::PrivacySettingOutData data;
            Core::hresult rc = p->GetPersonalization(data);
            p->Release();

            if (rc != Core::ERROR_NONE) {
                LOGWARN("PrivacyDelegate: GetPersonalization failed rc=%d", rc);
                return rc;
            }

            WPEFramework::Core::JSON::VariantContainer obj;
            obj["enabled"] = data.allowed;

            WPEFramework::Core::JSON::String serialized;
            obj.ToString(serialized);
            personalizationJson = serialized.Value();

            LOGINFO("PrivacyDelegate::GetPersonalizationAllowed => %s", personalizationJson.c_str());
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        /**
        * GetWatchHistoryAllowed
        * Retrieves watch history (share/remember) setting and returns:
        *   {"enabled": <bool>}
        */
        uint32_t GetWatchHistoryAllowed(std::string& watchHistoryJson) const {
            watchHistoryJson.clear();

            if (_shell == nullptr) {
                LOGERR("PrivacyDelegate: Shell not initialized");
                return Core::ERROR_UNAVAILABLE;
            }

            auto* p = _shell->QueryInterfaceByCallsign<WPEFramework::Exchange::IPrivacy>(PRIVACY_CALLSIGN);
            if (p == nullptr) {
                LOGWARN("PrivacyDelegate: IPrivacy (%s) not available", PRIVACY_CALLSIGN);
                return Core::ERROR_UNAVAILABLE;
            }

            WPEFramework::Exchange::IPrivacy::PrivacySettingOutData data;
            Core::hresult rc = p->GetWatchHistory(data);
            p->Release();

            if (rc != Core::ERROR_NONE) {
                LOGWARN("PrivacyDelegate: GetWatchHistory failed rc=%d", rc);
                return rc;
            }

            WPEFramework::Core::JSON::VariantContainer obj;
            obj["enabled"] = data.allowed;

            WPEFramework::Core::JSON::String serialized;
            obj.ToString(serialized);
            watchHistoryJson = serialized.Value();

            LOGINFO("PrivacyDelegate::GetWatchHistoryAllowed => %s", watchHistoryJson.c_str());
            return Core::ERROR_NONE;
        }

    private:
        PluginHost::IShell* _shell;
        mutable Core::CriticalSection mAdminLock;
    };
}  // namespace WPEFramework
