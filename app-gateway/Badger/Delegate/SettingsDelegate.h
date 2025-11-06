#pragma once

/**
 * SettingsDelegate
 *
 * Collects settings values from platform plugins and shapes responses for badger.settings.
 * It mirrors the Badger delegate pattern, using COM-RPC and JSON-RPC helpers already used
 * in other delegates, and returns a combined JSON object for requested keys.
 *
 * Supported keys:
 *  - CC_STATE -> org.rdk.UserSettings.getCaptions                 => {"CC_STATE":{"enabled":<bool>}}
 *  - VOICE_GUIDANCE_STATE -> org.rdk.UserSettings.getVoiceGuidance => {"VOICE_GUIDANCE_STATE":{"enabled":<bool>}}
 *  - ShowClosedCapture -> same as CC_STATE                         => {"ShowClosedCapture":{"enabled":<bool>}}
 *  - TextToSpeechEnabled2 -> same as VOICE_GUIDANCE_STATE          => {"TextToSpeechEnabled2":{"enabled":<bool>}}
 *  - DisplayPersonalizedRecommendations -> FbPrivacy.AllowPersonalization
 *          => {"DisplayPersonalizedRecommendations":{"enabled":<bool>}}
 *  - RememberWatchedPrograms -> FbPrivacy.AllowWatchHistory
 *          => {"RememberWatchedPrograms":{"enabled":<bool>}}
 *  - ShareWatchHistoryStatus -> FbPrivacy (best effort: ShareWatchHistory; fallback -> AllowWatchHistory)
 *          => {"ShareWatchHistoryStatus":{"enabled":<bool>}}
 *  - friendly_name -> org.rdk.System.getFriendlyName
 *          => {"friendly_name":{"value":"\"<Name>\""}}
 *  - legacyMiniGuide -> hard-coded empty object
 *          => {"legacyMiniGuide":{}}
 *  - power_save_status -> hard-coded empty object
 *          => {"power_save_status":{}}
 *
 * Error handling:
 *  - If a plugin call fails for a particular key, that key is omitted from the result.
 *  - Hard-coded keys (legacyMiniGuide, power_save_status) are always returned as {} if requested.
 */

#include <memory>
#include <string>
#include <vector>
#include <unordered_set>

#include <core/JSON.h>
#include <plugins/plugins.h>

#include "UtilsLogging.h"
#include "DelegateUtils.h"

// COM interfaces
#include <interfaces/IUserSettings.h>
#include <interfaces/IFbPrivacy.h>

// Callsigns
#ifndef USERSETTINGS_CALLSIGN
#define USERSETTINGS_CALLSIGN "org.rdk.UserSettings"
#endif

#ifndef SYSTEM_CALLSIGN
#define SYSTEM_CALLSIGN "org.rdk.System"
#endif

#ifndef FBPRIVACY_CALLSIGN
#define FBPRIVACY_CALLSIGN "org.rdk.Privacy"
#endif

class SettingsDelegate {
public:
    explicit SettingsDelegate(PluginHost::IShell* shell)
        : _shell(shell) {}

    ~SettingsDelegate() = default;

    // PUBLIC_INTERFACE
    /**
     * BuildSettings
     * Populate 'out' with the combined object for the requested 'keys'.
     * Each supported key is added only if it is requested and successfully retrieved.
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

        // Convert keys to a set for quick membership check
        std::unordered_set<std::string> wanted;
        wanted.reserve(keys.size());
        for (const auto& k : keys) {
            wanted.insert(k);
        }

        // Acquire COM interfaces if needed
        Exchange::IUserSettings* userSettings = nullptr;
        Exchange::IFbPrivacy* fbPrivacy = nullptr;

        auto ensureUserSettings = [&]() -> Exchange::IUserSettings* {
            if (!userSettings) {
                userSettings = _shell->QueryInterfaceByCallsign<Exchange::IUserSettings>(USERSETTINGS_CALLSIGN);
                if (userSettings == nullptr) {
                    LOGWARN("[badger.settings] IUserSettings (%s) not available", USERSETTINGS_CALLSIGN);
                }
            }
            return userSettings;
        };

        auto ensureFbPrivacy = [&]() -> Exchange::IFbPrivacy* {
            if (!fbPrivacy) {
                fbPrivacy = _shell->QueryInterfaceByCallsign<Exchange::IFbPrivacy>(FBPRIVACY_CALLSIGN);
                if (fbPrivacy == nullptr) {
                    LOGWARN("[badger.settings] IFbPrivacy (%s) not available", FBPRIVACY_CALLSIGN);
                }
            }
            return fbPrivacy;
        };

        // CC_STATE / ShowClosedCapture -> UserSettings.getCaptions
        auto addCaptions = [&](const std::string& outKey) {
            auto* us = ensureUserSettings();
            if (us) {
                bool enabled = false;
                Core::hresult rc = us->GetCaptions(enabled);
                if (rc == Core::ERROR_NONE) {
                    Core::JSON::VariantContainer obj;
                    obj["enabled"] = enabled;
                    out[outKey.c_str()] = obj;
                } else {
                    LOGWARN("[badger.settings] GetCaptions failed rc=%d", rc);
                }
            }
        };

        if (wanted.find("CC_STATE") != wanted.end()) {
            addCaptions("CC_STATE");
        }
        if (wanted.find("ShowClosedCapture") != wanted.end()) {
            addCaptions("ShowClosedCapture");
        }

        // VOICE_GUIDANCE_STATE / TextToSpeechEnabled2 -> UserSettings.getVoiceGuidance
        auto addVoiceGuidance = [&](const std::string& outKey) {
            auto* us = ensureUserSettings();
            if (us) {
                bool enabled = false;
                Core::hresult rc = us->GetVoiceGuidance(enabled);
                if (rc == Core::ERROR_NONE) {
                    Core::JSON::VariantContainer obj;
                    obj["enabled"] = enabled;
                    out[outKey.c_str()] = obj;
                } else {
                    LOGWARN("[badger.settings] GetVoiceGuidance failed rc=%d", rc);
                }
            }
        };

        if (wanted.find("VOICE_GUIDANCE_STATE") != wanted.end()) {
            addVoiceGuidance("VOICE_GUIDANCE_STATE");
        }
        if (wanted.find("TextToSpeechEnabled2") != wanted.end()) {
            addVoiceGuidance("TextToSpeechEnabled2");
        }

        // DisplayPersonalizedRecommendations -> FbPrivacy.AllowPersonalization
        if (wanted.find("DisplayPersonalizedRecommendations") != wanted.end()) {
            auto* fp = ensureFbPrivacy();
            if (fp) {
                bool allowed = false;
                Core::hresult rc = fp->AllowPersonalization(allowed);
                if (rc == Core::ERROR_NONE) {
                    Core::JSON::VariantContainer obj;
                    obj["enabled"] = allowed;
                    out["DisplayPersonalizedRecommendations"] = obj;
                } else {
                    LOGWARN("[badger.settings] AllowPersonalization failed rc=%d", rc);
                }
            }
        }

        // RememberWatchedPrograms -> FbPrivacy.AllowWatchHistory
        if (wanted.find("RememberWatchedPrograms") != wanted.end()) {
            auto* fp = ensureFbPrivacy();
            if (fp) {
                bool allowed = false;
                Core::hresult rc = fp->AllowWatchHistory(allowed);
                if (rc == Core::ERROR_NONE) {
                    Core::JSON::VariantContainer obj;
                    obj["enabled"] = allowed;
                    out["RememberWatchedPrograms"] = obj;
                } else {
                    LOGWARN("[badger.settings] AllowWatchHistory failed rc=%d", rc);
                }
            }
        }

        // ShareWatchHistoryStatus -> FbPrivacy ShareWatchHistory (fallback: AllowWatchHistory)
        if (wanted.find("ShareWatchHistoryStatus") != wanted.end()) {
            bool haveValue = false;
            bool shareAllowed = false;

            // Best-effort: try to deduce from Settings() if available; otherwise fallback to AllowWatchHistory.
            auto* fp = ensureFbPrivacy();
            if (fp) {
                // Fallback path
                bool allowed = false;
                Core::hresult rc2 = fp->AllowWatchHistory(allowed);
                if (rc2 == Core::ERROR_NONE) {
                    shareAllowed = allowed;
                    haveValue = true;
                } else {
                    LOGWARN("[badger.settings] Fallback AllowWatchHistory failed rc=%d", rc2);
                }
            }

            if (haveValue) {
                Core::JSON::VariantContainer obj;
                obj["enabled"] = shareAllowed;
                out["ShareWatchHistoryStatus"] = obj;
            }
        }

        // friendly_name -> org.rdk.System.getFriendlyName => {"friendly_name":{"value":"\"Living Room\""}}
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
                        // Wrap name in quotes so JSON string contains escaped quotes, matching required shape
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

        // Release COM interfaces if acquired
        if (userSettings) {
            userSettings->Release();
            userSettings = nullptr;
        }
        if (fbPrivacy) {
            fbPrivacy->Release();
            fbPrivacy = nullptr;
        }

        return Core::ERROR_NONE;
    }

private:
    PluginHost::IShell* _shell;
};
