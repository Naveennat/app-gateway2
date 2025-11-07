#pragma once

/**
 * SettingsDelegate (System-only)
 *
 * Focused exclusively on SYSTEM_CALLSIGN-related settings dispatch and system-scoped keys.
 * Previously handled UserSettings and Privacy logic has been moved to:
 *   - UserSettingsDelegate.h (org.rdk.UserSettings)
 *   - PrivacyDelegate.h (org.rdk.Privacy)
 *
 * Supported keys in BuildSettings:
 *  - friendly_name      -> org.rdk.System.getFriendlyName => {"friendly_name":{"value":"\"<Name>\""}}
 *  - legacyMiniGuide    -> hard-coded empty object         => {"legacyMiniGuide":{}}
 *  - power_save_status  -> hard-coded empty object         => {"power_save_status":{}}
 */

#include <memory>
#include <string>
#include <vector>
#include <unordered_set>

#include <core/JSON.h>
#include <plugins/plugins.h>

#include "UtilsLogging.h"
#include "DelegateUtils.h"

// Callsign
#ifndef SYSTEM_CALLSIGN
#define SYSTEM_CALLSIGN "org.rdk.System"
#endif

class SettingsDelegate {
public:
    explicit SettingsDelegate(PluginHost::IShell* shell)
        : _shell(shell) {}

    ~SettingsDelegate() = default;

    // PUBLIC_INTERFACE
    /**
     * BuildSettings
     * Populate 'out' with the combined object for the requested 'keys' for system-only keys.
     * Returns Core::ERROR_NONE on success, Core::ERROR_UNAVAILABLE if shell is missing.
     */
    Core::hresult BuildSettings(const std::vector<std::string>& keys,
                                WPEFramework::Core::JSON::VariantContainer& out) const
    {
        using namespace WPEFramework;

        if (_shell == nullptr) {
            LOGERR("[badger.settings] Shell not initialized");
            return Core::ERROR_UNAVAILABLE;
        }

        std::unordered_set<std::string> wanted;
        wanted.reserve(keys.size());
        for (const auto& k : keys) {
            wanted.insert(k);
        }

        // friendly_name -> org.rdk.System.getFriendlyName => {"friendly_name":{"value":"\"<Name>\""}}
        if (wanted.find("friendly_name") != wanted.end()) {
            auto link = DelegateUtils::AcquireLink(_shell, SYSTEM_CALLSIGN);
            if (link) {
                JsonObject params;
                JsonObject response;
                uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("getFriendlyName"), params, response);
                if (rc == Core::ERROR_NONE) {
                    if (response.HasLabel("friendlyName")) {
                        std::string friendly = response["friendlyName"].String();
                        Core::JSON::VariantContainer obj;
                        // Wrap in quotes so JSON string contains escaped quotes, matching required shape
                        std::string wrapped = std::string("\"") + friendly + std::string("\"");
                        obj["value"] = wrapped;
                        out["friendly_name"] = obj;
                    } else {
                        LOGWARN("[badger.settings] getFriendlyName missing 'friendlyName' field");
                    }
                } else {
                    LOGWARN("[badger.settings] getFriendlyName failed rc=%u", rc);
                }
            } else {
                LOGWARN("[badger.settings] No JSONRPC link for %s", SYSTEM_CALLSIGN);
            }
        }

        // legacyMiniGuide -> {}
        if (wanted.find("legacyMiniGuide") != wanted.end()) {
            Core::JSON::VariantContainer emptyObj;
            out["legacyMiniGuide"] = emptyObj;
        }

        // power_save_status -> {}
        if (wanted.find("power_save_status") != wanted.end()) {
            Core::JSON::VariantContainer emptyObj;
            out["power_save_status"] = emptyObj;
        }

        return Core::ERROR_NONE;
    }

private:
    PluginHost::IShell* _shell;
};
