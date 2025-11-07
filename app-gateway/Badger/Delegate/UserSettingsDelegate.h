#pragma once

/**
 * UserSettingsDelegate
 *
 * Encapsulates interactions with the org.rdk.UserSettings plugin (Exchange::IUserSettings)
 * and exposes simple helpers that serialize results into JSON strings to match the
 * pattern used by other delegates (e.g., SystemDelegate).
 */

#include <memory>
#include <string>

#include <core/JSON.h>
#include <plugins/plugins.h>

#include "UtilsLogging.h"

// COM interface
#include <interfaces/IUserSettings.h>

// Callsign
#ifndef USERSETTINGS_CALLSIGN
#define USERSETTINGS_CALLSIGN "org.rdk.UserSettings"
#endif

class UserSettingsDelegate {
public:
    // PUBLIC_INTERFACE
    explicit UserSettingsDelegate(PluginHost::IShell* shell)
        : _shell(shell) {}

    ~UserSettingsDelegate() = default;

    // PUBLIC_INTERFACE
    /**
     * GetCaptionsEnabled
     * Retrieves captions setting from IUserSettings and returns:
     *   {"enabled": <bool>}
     */
    uint32_t GetCaptionsEnabled(std::string& captionsJson) const {
        captionsJson.clear();

        if (_shell == nullptr) {
            LOGERR("UserSettingsDelegate: Shell not initialized");
            return Core::ERROR_UNAVAILABLE;
        }

        auto* us = _shell->QueryInterfaceByCallsign<WPEFramework::Exchange::IUserSettings>(USERSETTINGS_CALLSIGN);
        if (us == nullptr) {
            LOGWARN("UserSettingsDelegate: IUserSettings (%s) not available", USERSETTINGS_CALLSIGN);
            return Core::ERROR_UNAVAILABLE;
        }

        bool enabled = false;
        Core::hresult rc = us->GetCaptions(enabled);
        us->Release();

        if (rc != Core::ERROR_NONE) {
            LOGWARN("UserSettingsDelegate: GetCaptions failed rc=%d", rc);
            return rc;
        }

        WPEFramework::Core::JSON::VariantContainer obj;
        obj["enabled"] = enabled;

        WPEFramework::Core::JSON::String serialized;
        obj.ToString(serialized);
        captionsJson = serialized.Value();

        LOGINFO("UserSettingsDelegate::GetCaptionsEnabled => %s", captionsJson.c_str());
        return Core::ERROR_NONE;
    }

    // PUBLIC_INTERFACE
    /**
     * GetVoiceGuidanceState
     * Retrieves voice guidance setting from IUserSettings and returns:
     *   {"enabled": <bool>}
     */
    uint32_t GetVoiceGuidanceState(std::string& voiceGuidanceJson) const {
        voiceGuidanceJson.clear();

        if (_shell == nullptr) {
            LOGERR("UserSettingsDelegate: Shell not initialized");
            return Core::ERROR_UNAVAILABLE;
        }

        auto* us = _shell->QueryInterfaceByCallsign<WPEFramework::Exchange::IUserSettings>(USERSETTINGS_CALLSIGN);
        if (us == nullptr) {
            LOGWARN("UserSettingsDelegate: IUserSettings (%s) not available", USERSETTINGS_CALLSIGN);
            return Core::ERROR_UNAVAILABLE;
        }

        bool enabled = false;
        Core::hresult rc = us->GetVoiceGuidance(enabled);
        us->Release();

        if (rc != Core::ERROR_NONE) {
            LOGWARN("UserSettingsDelegate: GetVoiceGuidance failed rc=%d", rc);
            return rc;
        }

        WPEFramework::Core::JSON::VariantContainer obj;
        obj["enabled"] = enabled;

        WPEFramework::Core::JSON::String serialized;
        obj.ToString(serialized);
        voiceGuidanceJson = serialized.Value();

        LOGINFO("UserSettingsDelegate::GetVoiceGuidanceState => %s", voiceGuidanceJson.c_str());
        return Core::ERROR_NONE;
    }

private:
    PluginHost::IShell* _shell;
    mutable Core::CriticalSection mAdminLock;
};
