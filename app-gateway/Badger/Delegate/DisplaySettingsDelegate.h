/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

#pragma once

#include "DelegateUtils.h"

// Define a callsign constant to match the AUTHSERVICE_CALLSIGN-style pattern.
#ifndef DISPLAY_SETTINGS_CALLSIGN
#define DISPLAY_SETTINGS_CALLSIGN "org.rdk.DisplaySettings"
#endif

namespace WPEFramework {
    class DisplaySettingsDelegate {
      public:
        DisplaySettingsDelegate(PluginHost::IShell* shell) : _shell(shell) {}

        ~DisplaySettingsDelegate() = default;

      public:
        uint32_t GetAudioFormat(std::string& audioModesJson) {
            audioModesJson.clear();
            auto link = DelegateUtils::AcquireLink(_shell, DISPLAY_SETTINGS_CALLSIGN);
            if (!link) {
                return Core::ERROR_UNAVAILABLE;
            }

            JsonObject params;
            JsonObject response;

            uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("getAudioFormat"), params, response);
            if (rc != Core::ERROR_NONE) {
                LOGERR("getAudioFormat failed, rc=%u", rc);
                return rc;
            }

            Core::JSON::VariantContainer result;
            Core::JSON::VariantContainer supportedList;

            // currentAudioFormat
            if (response.HasLabel("currentAudioFormat")) {
                result["current_audio_mode"] = response["currentAudioFormat"].String();
            } else {
                result["current_audio_mode"] = "UNKNOWN";
            }

            // supportedAudioFormat[]
            if (response.HasLabel("supportedAudioFormat")) {
                const JsonArray& arr = response["supportedAudioFormat"].Array();
                for (uint32_t i = 0; i < arr.Length(); ++i) {
                    std::string indexKey = std::to_string(i);
                    supportedList[indexKey.c_str()] = arr[i].String();
                }
            }

            result["supported_audio_modes"] = supportedList;
            result.ToString(audioModesJson);

            LOGINFO("AudioFormat JSON: %s", audioModesJson.c_str());
            return Core::ERROR_NONE;
        }

        uint32_t GetHDRSupport(std::string& hdrJson) {
            hdrJson.clear();
            auto link = DelegateUtils::AcquireLink(_shell, DISPLAY_SETTINGS_CALLSIGN);
            if (!link) {
                LOGERR("displaySettingsLink is null in GetHDRSupport()");
                return Core::ERROR_UNAVAILABLE;
            }

            JsonObject params;
            JsonObject response;

            Core::JSON::VariantContainer result;
            Core::JSON::VariantContainer settopProfile;
            Core::JSON::VariantContainer tvProfile;

            auto fillProfile = [&](const JsonObject& resp, Core::JSON::VariantContainer& profileVC) {
                Core::JSON::VariantContainer temp;
                temp["supportsHDR"] = resp.HasLabel("supportsHDR") ? resp["supportsHDR"].Boolean() : false;

                if (resp.HasLabel("standards")) {
                    Core::JSON::VariantContainer standardsList;
                    const JsonArray& arr = resp["standards"].Array();
                    for (uint32_t i = 0; i < arr.Length(); ++i) {
                        std::string indexKey = std::to_string(i);
                        standardsList[indexKey.c_str()] = arr[i].String();
                    }
                    temp["standards"] = standardsList;
                }

                profileVC = temp;
            };

            // ---- Settop HDR Support ----
            response.Clear();
            uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("getSettopHDRSupport"), params, response);
            if (rc == Core::ERROR_NONE) {
                fillProfile(response, settopProfile);
            } else {
                LOGERR("getSettopHDRSupport failed, rc=%u", rc);
            }

            // ---- TV HDR Support ----
            response.Clear();
            rc = link->Invoke<JsonObject, JsonObject>(_T("getTvHDRSupport"), params, response);
            if (rc == Core::ERROR_NONE) {
                fillProfile(response, tvProfile);
            } else {
                LOGERR("getTvHDRSupport failed, rc=%u", rc);
            }

            result["settop_hdr_support"] = settopProfile;
            result["tv_hdr_support"] = tvProfile;
            result.ToString(hdrJson);

            LOGINFO("HDR JSON: %s", hdrJson.c_str());
            return Core::ERROR_NONE;
        }

      private:
        PluginHost::IShell* _shell;
    };
}  // namespace WPEFramework
