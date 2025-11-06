#pragma once

/*
 * AudioUtils.h
 *
 * Helpers for normalizing HDMI audio format strings from Thunder DisplaySettings
 * and converting them into Firebolt device.audio capability JSON payloads.
 *
 * This implements the required jq-like transform in C++:
 * - Normalize plugin values:
 *   DOLBY AC3   -> Dolby Digital
 *   DOLBY EAC3  -> Dolby Digital Plus
 *   any *ATMOS* -> Dolby Atmos
 *   PCM/STEREO  -> Stereo
 * - Build capability object booleans from normalized format with a safe fallback.
 */

#include <string>
#include <algorithm>
#include <core/JSON.h>

namespace WPEFramework {
namespace Plugin {

    // PUBLIC_INTERFACE
    inline std::string NormalizeHdmiAudioFormat(const std::string& pluginValue) {
        /**
         * Normalize DisplaySettings audio format into canonical names:
         * - DOLBY AC3   -> "Dolby Digital"
         * - DOLBY EAC3  -> "Dolby Digital Plus"
         * - *ATMOS*     -> "Dolby Atmos"
         * - PCM/STEREO  -> "Stereo"
         * - Otherwise   -> "Unknown"
         */
        std::string in = pluginValue;
        // Trim spaces
        auto ltrim = [](std::string& s) {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch){ return !std::isspace(ch); }));
        };
        auto rtrim = [](std::string& s) {
            s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch){ return !std::isspace(ch); }).base(), s.end());
        };
        ltrim(in);
        rtrim(in);

        std::string u(in);
        std::transform(u.begin(), u.end(), u.begin(), [](unsigned char c){ return static_cast<char>(::toupper(c)); });

        if (u.find("ATMOS") != std::string::npos) {
            return "Dolby Atmos";
        }
        if (u.find("EAC3") != std::string::npos || u.find("DD+") != std::string::npos || u.find("DOLBY DIGITAL PLUS") != std::string::npos) {
            return "Dolby Digital Plus";
        }
        if (u.find("AC3") != std::string::npos || u.find("DD") != std::string::npos || u.find("DOLBY DIGITAL") != std::string::npos) {
            return "Dolby Digital";
        }
        if (u.find("PCM") != std::string::npos || u.find("STEREO") != std::string::npos) {
            return "Stereo";
        }
        return "Unknown";
    }

    // PUBLIC_INTERFACE
    inline std::string BuildAudioCapabilityJsonFromFormat(const std::string& normalizedFormat) {
        /**
         * Build Firebolt device.audio capability object from normalized format.
         * Output JSON with fields:
         *  - stereo
         *  - dolbyAtmos
         *  - dolbyDigital5.1
         *  - dolbyDigital5.1+
         * Fallback (Unknown): stereo=true, all others=false.
         */
        bool stereo = false, atmos = false, dd51 = false, dd51p = false;

        if (normalizedFormat == "Dolby Atmos") {
            atmos = true;
        } else if (normalizedFormat == "Dolby Digital Plus") {
            dd51p = true;
        } else if (normalizedFormat == "Dolby Digital") {
            dd51 = true;
        } else if (normalizedFormat == "Stereo") {
            stereo = true;
        } else {
            // Fallback for unknown/empty formats
            stereo = true;
        }

        // Use VariantContainer to construct JSON object consistently with Thunder APIs.
        WPEFramework::Core::JSON::VariantContainer obj;
        obj["stereo"] = stereo;
        obj["dolbyAtmos"] = atmos;
        obj["dolbyDigital5.1"] = dd51;
        obj["dolbyDigital5.1+"] = dd51p;

        std::string json;
        obj.ToString(json);
        return json;
    }

} // namespace Plugin
} // namespace WPEFramework
