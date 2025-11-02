#pragma once

/*
 * SystemDelegate encapsulates all org.rdk.System JSON-RPC calls and event subscriptions
 * required by the FbSettings plugin to fulfill the system settings alias functions.
 *
 * It uses a direct JSON-RPC link to the Thunder plugin via Utils::GetThunderControllerClient,
 * following the same pattern as the AUTHSERVICE_CALLSIGN-style setup (using helpers from Supporting_Files).
 */

#include <memory>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <cstdlib>
#include <cerrno>
#include <cstdint>

#include <plugins/plugins.h>
#include <core/JSON.h>
#include "UtilsLogging.h"
#include "UtilsJsonrpcDirectLink.h"
#include "ThunderUtils.h"
#include "BaseEventDelegate.h"

using namespace WPEFramework;

// Define a callsign constant to match the AUTHSERVICE_CALLSIGN-style pattern.
#ifndef SYSTEM_CALLSIGN
#define SYSTEM_CALLSIGN "org.rdk.System"
#endif

#ifndef DISPLAYSETTINGS_CALLSIGN
#define DISPLAYSETTINGS_CALLSIGN "org.rdk.DisplaySettings"
#endif

#ifndef HDCPPROFILE_CALLSIGN
#define HDCPPROFILE_CALLSIGN "org.rdk.HdcpProfile"
#endif

class SystemDelegate : public BaseEventDelegate
{

    // Event names exposed by this delegate (consumer subscriptions may vary in case)
    static constexpr const char* EVENT_ON_VIDEO_RES_CHANGED   = "device.onVideoResolutionChanged";
    static constexpr const char* EVENT_ON_SCREEN_RES_CHANGED  = "device.onScreenResolutionChanged";
    static constexpr const char* EVENT_ON_HDR_CHANGED         = "device.onHdrChanged";
    static constexpr const char* EVENT_ON_HDCP_CHANGED        = "device.onHdcpChanged";

#ifdef ENABLE_DEBOUNCE
    // Debounce configuration (compile-time only).
    // Fixed window of 150ms when enabled.
    static constexpr uint32_t kDefaultDebounceMs = 150;

    // PUBLIC_INTERFACE
    static inline std::chrono::milliseconds DebounceWindow() {
        return std::chrono::milliseconds(kDefaultDebounceMs);
    }
#endif

public:
    SystemDelegate(PluginHost::IShell *shell, Exchange::IAppNotifications* appNotifications)
        : BaseEventDelegate(appNotifications)
        , _shell(shell)
    {
    }

    ~SystemDelegate() = default;

    // PUBLIC_INTERFACE
    Core::hresult GetDeviceMake(std::string &make)
    {
        /** Retrieve the device make using org.rdk.System.getDeviceInfo */
        LOGINFO("GetDeviceMake FbSettings Delegate");
        make.clear();
        auto link = AcquireLink();
        if (!link)
        {
            make = "unknown";
            return Core::ERROR_UNAVAILABLE;
        }

        WPEFramework::Core::JSON::VariantContainer params;
        WPEFramework::Core::JSON::VariantContainer response;
        const uint32_t rc = link->Invoke<WPEFramework::Core::JSON::VariantContainer, WPEFramework::Core::JSON::VariantContainer>("getDeviceInfo", params, response);
        if (rc == Core::ERROR_NONE)
        {
            if (response.HasLabel(_T("make")))
            {
                make = response[_T("make")].String();
            }
        }

        if (make.empty())
        {
            // Per transform: return_or_else(.result.make, "unknown")
            make = "unknown";
        }
        return Core::ERROR_NONE;
    }

    // PUBLIC_INTERFACE
    Core::hresult GetDeviceName(std::string &name)
    {
        /** Retrieve the friendly name using org.rdk.System.getFriendlyName */
        name.clear();
        auto link = AcquireLink();
        if (!link)
        {
            name = "Living Room";
            return Core::ERROR_UNAVAILABLE;
        }

        WPEFramework::Core::JSON::VariantContainer params;
        WPEFramework::Core::JSON::VariantContainer response;
        const uint32_t rc = link->Invoke<WPEFramework::Core::JSON::VariantContainer, WPEFramework::Core::JSON::VariantContainer>("getFriendlyName", params, response);
        if (rc == Core::ERROR_NONE && response.HasLabel(_T("friendlyName")))
        {
            name = response[_T("friendlyName")].String();
        }

        // Default if empty
        if (name.empty())
        {
            name = "Living Room";
        }
        return Core::ERROR_NONE;
    }

    // PUBLIC_INTERFACE
    Core::hresult SetDeviceName(const std::string &name)
    {
        /** Set the friendly name using org.rdk.System.setFriendlyName */
        auto link = AcquireLink();
        if (!link)
        {
            return Core::ERROR_UNAVAILABLE;
        }

        WPEFramework::Core::JSON::VariantContainer params;
        params[_T("friendlyName")] = name;
        WPEFramework::Core::JSON::VariantContainer response;
        const uint32_t rc = link->Invoke<WPEFramework::Core::JSON::VariantContainer, WPEFramework::Core::JSON::VariantContainer>("setFriendlyName", params, response);
        if (rc == Core::ERROR_NONE && response.HasLabel(_T("success")) && response[_T("success")].Boolean())
        {
            return Core::ERROR_NONE;
        }
        LOGERR("SystemDelegate: couldn't set name");
        return Core::ERROR_GENERAL;
    }

    // PUBLIC_INTERFACE
    Core::hresult GetDeviceSku(std::string &skuOut)
    {
        /** Retrieve the device SKU from org.rdk.System.getSystemVersions.stbVersion */
        skuOut.clear();
        auto link = AcquireLink();
        if (!link)
        {
            return Core::ERROR_UNAVAILABLE;
        }

        WPEFramework::Core::JSON::VariantContainer params;
        WPEFramework::Core::JSON::VariantContainer response;
        const uint32_t rc = link->Invoke<WPEFramework::Core::JSON::VariantContainer, WPEFramework::Core::JSON::VariantContainer>("getSystemVersions", params, response);
        if (rc != Core::ERROR_NONE)
        {
            LOGERR("SystemDelegate: getSystemVersions failed rc=%u", rc);
            return Core::ERROR_UNAVAILABLE;
        }
        if (!response.HasLabel(_T("stbVersion")))
        {
            LOGERR("SystemDelegate: getSystemVersions missing stbVersion");
            return Core::ERROR_UNAVAILABLE;
        }

        const std::string stbVersion = response[_T("stbVersion")].String();
        // Per transform: split("_")[0]
        auto pos = stbVersion.find('_');
        skuOut = (pos == std::string::npos) ? stbVersion : stbVersion.substr(0, pos);
        if (skuOut.empty())
        {
            LOGERR("SystemDelegate: Failed to get SKU");
            return Core::ERROR_UNAVAILABLE;
        }
        return Core::ERROR_NONE;
    }

    // PUBLIC_INTERFACE
    Core::hresult GetCountryCode(std::string &code)
    {
        /** Retrieve Firebolt country code derived from org.rdk.System.getTerritory */
        code.clear();
        auto link = AcquireLink();
        if (!link)
        {
            code = "US";
            return Core::ERROR_UNAVAILABLE;
        }

        WPEFramework::Core::JSON::VariantContainer params;
        WPEFramework::Core::JSON::VariantContainer response;
        const uint32_t rc = link->Invoke<WPEFramework::Core::JSON::VariantContainer, WPEFramework::Core::JSON::VariantContainer>("getTerritory", params, response);
        if (rc == Core::ERROR_NONE && response.HasLabel(_T("territory")))
        {
            const std::string terr = response[_T("territory")].String();
            code = TerritoryThunderToFirebolt(terr, "US");
        }
        if (code.empty())
        {
            code = "US";
        }
        return Core::ERROR_NONE;
    }

    // PUBLIC_INTERFACE
    Core::hresult SetCountryCode(const std::string &code)
    {
        /** Set territory using org.rdk.System.setTerritory mapped from Firebolt country code */
        auto link = AcquireLink();
        if (!link)
        {
            return Core::ERROR_UNAVAILABLE;
        }

        const std::string territory = TerritoryFireboltToThunder(code, "USA");
        WPEFramework::Core::JSON::VariantContainer params;
        params[_T("territory")] = territory;

        WPEFramework::Core::JSON::VariantContainer response;
        const uint32_t rc = link->Invoke<WPEFramework::Core::JSON::VariantContainer, WPEFramework::Core::JSON::VariantContainer>("setTerritory", params, response);
        if (rc == Core::ERROR_NONE && response.HasLabel(_T("success")) && response[_T("success")].Boolean())
        {
            return Core::ERROR_NONE;
        }
        LOGERR("SystemDelegate: couldn't set countrycode");
        return Core::ERROR_GENERAL;
    }

    // PUBLIC_INTERFACE
    Core::hresult GetTimeZone(std::string &tz)
    {
        /** Retrieve timezone using org.rdk.System.getTimeZoneDST */
        tz.clear();
        auto link = AcquireLink();
        if (!link)
        {
            return Core::ERROR_UNAVAILABLE;
        }

        WPEFramework::Core::JSON::VariantContainer params;
        WPEFramework::Core::JSON::VariantContainer response;
        const uint32_t rc = link->Invoke<WPEFramework::Core::JSON::VariantContainer, WPEFramework::Core::JSON::VariantContainer>("getTimeZoneDST", params, response);
        if (rc == Core::ERROR_NONE && response.HasLabel(_T("success")) && response[_T("success")].Boolean())
        {
            if (response.HasLabel(_T("timeZone")))
            {
                tz = response[_T("timeZone")].String();
                return Core::ERROR_NONE;
            }
        }
        LOGERR("SystemDelegate: couldn't get timezone");
        return Core::ERROR_UNAVAILABLE;
    }

    // PUBLIC_INTERFACE
    Core::hresult SetTimeZone(const std::string &tz)
    {
        /** Set timezone using org.rdk.System.setTimeZoneDST */
        auto link = AcquireLink();
        if (!link)
        {
            return Core::ERROR_UNAVAILABLE;
        }

        WPEFramework::Core::JSON::VariantContainer params;
        params[_T("timeZone")] = tz;
        WPEFramework::Core::JSON::VariantContainer response;
        const uint32_t rc = link->Invoke<WPEFramework::Core::JSON::VariantContainer, WPEFramework::Core::JSON::VariantContainer>("setTimeZoneDST", params, response);
        if (rc == Core::ERROR_NONE && response.HasLabel(_T("success")) && response[_T("success")].Boolean())
        {
            return Core::ERROR_NONE;
        }
        LOGERR("SystemDelegate: couldn't set timezone");
        return Core::ERROR_GENERAL;
    }

    // PUBLIC_INTERFACE
    Core::hresult GetSecondScreenFriendlyName(std::string &name)
    {
        /** Alias to GetDeviceName using org.rdk.System.getFriendlyName */
        return GetDeviceName(name);
    }

    // ---- New methods: Video/Screen Resolution, HDCP, HDR ----

    // PUBLIC_INTERFACE
    Core::hresult GetScreenResolution(std::string &jsonArray)
    {
        /**
         * Get [w, h] screen resolution using DisplaySettings.getCurrentResolution.
         * Returns "[1920,1080]" as fallback when unavailable.
         */
        jsonArray = "[1920,1080]";
        auto link = AcquireLink(DISPLAYSETTINGS_CALLSIGN);
        if (!link) {
            return Core::ERROR_UNAVAILABLE;
        }

        WPEFramework::Core::JSON::VariantContainer params;
        WPEFramework::Core::JSON::VariantContainer response;
        const uint32_t rc = link->Invoke<WPEFramework::Core::JSON::VariantContainer, WPEFramework::Core::JSON::VariantContainer>("getCurrentResolution", params, response);
        if (rc != Core::ERROR_NONE) {
            return Core::ERROR_GENERAL;
        }

        int w = 1920, h = 1080;

        // Try top-level first
        if (response.HasLabel(_T("w")) && response.HasLabel(_T("h"))) {
            auto wv = response.Get(_T("w"));
            auto hv = response.Get(_T("h"));
            if (wv.Content() == WPEFramework::Core::JSON::Variant::type::NUMBER &&
                hv.Content() == WPEFramework::Core::JSON::Variant::type::NUMBER) {
                w = static_cast<int>(wv.Number());
                h = static_cast<int>(hv.Number());
            }
        } else if (response.HasLabel(_T("result"))) {
            // Try nested "result"
            auto r = response.Get(_T("result"));
            if (r.Content() == WPEFramework::Core::JSON::Variant::type::OBJECT) {
                auto wnv = r.Object().Get(_T("w"));
                auto hnv = r.Object().Get(_T("h"));
                auto wdv = r.Object().Get(_T("width"));
                auto hdv = r.Object().Get(_T("height"));

                if (wnv.Content() == WPEFramework::Core::JSON::Variant::type::NUMBER &&
                    hnv.Content() == WPEFramework::Core::JSON::Variant::type::NUMBER) {
                    w = static_cast<int>(wnv.Number());
                    h = static_cast<int>(hnv.Number());
                } else if (wdv.Content() == WPEFramework::Core::JSON::Variant::type::NUMBER &&
                           hdv.Content() == WPEFramework::Core::JSON::Variant::type::NUMBER) {
                    w = static_cast<int>(wdv.Number());
                    h = static_cast<int>(hdv.Number());
                }
            }
        }

        jsonArray = "[" + std::to_string(w) + "," + std::to_string(h) + "]";
        LOGDBG("[FbSettings|ScreenResolutionChanged] Computed screenResolution: w=%d h=%d -> %s", w, h, jsonArray.c_str());
        return Core::ERROR_NONE;
    }

    // PUBLIC_INTERFACE
    Core::hresult GetVideoResolution(std::string &jsonArray)
    {
        /**
         * Get [w, h] video resolution. Prefer DisplaySettings.getCurrentResolution width
         * to infer UHD vs FHD; else default to 1080p.
         * This is a stubbed approximation of the /system/hdmi.uhdConfigured logic.
         */
        std::string sr;
        (void)GetScreenResolution(sr);
        // sr expected format: "[w,h]"
        int w = 1920, h = 1080;
        if (sr.size() > 2 && sr.front() == '[' && sr.back() == ']') {
            try {
                // Simple parsing without heavy JSON dependencies
                auto comma = sr.find(',');
                if (comma != std::string::npos) {
                    int sw = std::stoi(sr.substr(1, comma - 1));
                    int sh = std::stoi(sr.substr(comma + 1, sr.size() - comma - 2));
                    if (sw >= 3840 || sh >= 2160) {
                        w = 3840; h = 2160;
                    } else {
                        w = 1920; h = 1080;
                    }
                    LOGDBG("[FbSettings|VideoResolutionChanged] Transform screen(%d x %d) -> video(%d x %d)", sw, sh, w, h);
                }
            } catch (...) {
                // keep defaults
                LOGDBG("[FbSettings|VideoResolutionChanged] Transform parse error for %s -> using defaults (%d x %d)", sr.c_str(), w, h);
            }
        }
        jsonArray = "[" + std::to_string(w) + "," + std::to_string(h) + "]";
        return Core::ERROR_NONE;
    }

    // PUBLIC_INTERFACE
    Core::hresult GetHdcp(std::string &jsonObject)
    {
        /**
         * Get HDCP status via HdcpProfile.getHDCPStatus.
         * Return {"hdcp1.4":bool,"hdcp2.2":bool} with sensible defaults.
         */
        jsonObject = "{\"hdcp1.4\":false,\"hdcp2.2\":false}";
        auto link = AcquireLink(HDCPPROFILE_CALLSIGN);
        if (!link) {
            return Core::ERROR_UNAVAILABLE;
        }

        WPEFramework::Core::JSON::VariantContainer params;
        WPEFramework::Core::JSON::VariantContainer response;
        const uint32_t rc = link->Invoke<WPEFramework::Core::JSON::VariantContainer, WPEFramework::Core::JSON::VariantContainer>("getHDCPStatus", params, response);
        if (rc != Core::ERROR_NONE) {
            return Core::ERROR_GENERAL;
        }

        bool hdcp14 = false;
        bool hdcp22 = false;

        // Prefer nested "result" if available
        if (response.HasLabel(_T("result"))) {
            auto r = response.Get(_T("result"));
            if (r.Content() == WPEFramework::Core::JSON::Variant::type::OBJECT) {
                auto succ = r.Object().Get(_T("success"));
                auto status = r.Object().Get(_T("HDCPStatus"));
                if (succ.Content() == WPEFramework::Core::JSON::Variant::type::BOOLEAN && succ.Boolean() &&
                    status.Content() == WPEFramework::Core::JSON::Variant::type::OBJECT) {
                    auto reason = status.Object().Get(_T("hdcpReason"));
                    auto version = status.Object().Get(_T("currentHDCPVersion"));
                    if (reason.Content() == WPEFramework::Core::JSON::Variant::type::NUMBER &&
                        static_cast<int>(reason.Number()) == 2 &&
                        version.Content() == WPEFramework::Core::JSON::Variant::type::STRING) {
                        const std::string v = version.String();
                        if (v == "1.4") { hdcp14 = true; }
                        else { hdcp22 = true; }
                    }
                }
            }
        } else {
            // Fallback: try top-level fields if present
            auto status = response.Get(_T("HDCPStatus"));
            if (status.Content() == WPEFramework::Core::JSON::Variant::type::OBJECT) {
                auto reason = status.Object().Get(_T("hdcpReason"));
                auto version = status.Object().Get(_T("currentHDCPVersion"));
                if (reason.Content() == WPEFramework::Core::JSON::Variant::type::NUMBER &&
                    static_cast<int>(reason.Number()) == 2 &&
                    version.Content() == WPEFramework::Core::JSON::Variant::type::STRING) {
                    const std::string v = version.String();
                    if (v == "1.4") { hdcp14 = true; }
                    else { hdcp22 = true; }
                }
            }
        }

        jsonObject = std::string("{\"hdcp1.4\":") + (hdcp14 ? "true" : "false")
                   + ",\"hdcp2.2\":" + (hdcp22 ? "true" : "false") + "}";
        LOGDBG("[FbSettings|HdcpChanged] Computed HDCP flags: hdcp1.4=%s hdcp2.2=%s -> %s",
               hdcp14 ? "true" : "false", hdcp22 ? "true" : "false", jsonObject.c_str());
        return Core::ERROR_NONE;
    }

    // PUBLIC_INTERFACE
    Core::hresult GetHdr(std::string &jsonObject)
    {
        /**
         * Retrieve HDR capability/state via DisplaySettings.getTVHDRCapabilities.
         * Returns object with hdr10, dolbyVision, hlg, hdr10Plus flags (defaults false).
         */
        jsonObject = "{\"hdr10\":false,\"dolbyVision\":false,\"hlg\":false,\"hdr10Plus\":false}";
        auto link = AcquireLink(DISPLAYSETTINGS_CALLSIGN);
        if (!link) {
            return Core::ERROR_UNAVAILABLE;
        }

        WPEFramework::Core::JSON::VariantContainer params;
        WPEFramework::Core::JSON::VariantContainer response;
        const uint32_t rc = link->Invoke<WPEFramework::Core::JSON::VariantContainer, WPEFramework::Core::JSON::VariantContainer>("getTVHDRCapabilities", params, response);
        if (rc != Core::ERROR_NONE) {
            return Core::ERROR_GENERAL;
        }

        bool hdr10 = false, dv = false, hlg = false, hdr10plus = false;

        auto parseCaps = [&](const WPEFramework::Core::JSON::Variant& vobj) {
            if (vobj.Content() == WPEFramework::Core::JSON::Variant::type::OBJECT) {
                auto hdmi = vobj.Object().Get(_T("hdmi"));
                if (hdmi.Content() == WPEFramework::Core::JSON::Variant::type::OBJECT) {
                    auto sinkHDR10 = hdmi.Object().Get(_T("sinkHDR10"));
                    auto sinkDolbyVision = hdmi.Object().Get(_T("sinkDolbyVision"));
                    auto sinkHLG = hdmi.Object().Get(_T("sinkHLG"));
                    auto sinkHDR10Plus = hdmi.Object().Get(_T("sinkHDR10Plus"));
                    hdr10    = (sinkHDR10.Content() == WPEFramework::Core::JSON::Variant::type::BOOLEAN) ? sinkHDR10.Boolean() : false;
                    dv       = (sinkDolbyVision.Content() == WPEFramework::Core::JSON::Variant::type::BOOLEAN) ? sinkDolbyVision.Boolean() : false;
                    hlg      = (sinkHLG.Content() == WPEFramework::Core::JSON::Variant::type::BOOLEAN) ? sinkHLG.Boolean() : false;
                    hdr10plus= (sinkHDR10Plus.Content() == WPEFramework::Core::JSON::Variant::type::BOOLEAN) ? sinkHDR10Plus.Boolean() : false;
                }
            }
        };

        if (response.HasLabel(_T("result"))) {
            auto r = response.Get(_T("result"));
            parseCaps(r);
        } else {
            parseCaps(response);
        }

        jsonObject = std::string("{\"hdr10\":") + (hdr10 ? "true" : "false")
                   + ",\"dolbyVision\":" + (dv ? "true" : "false")
                   + ",\"hlg\":" + (hlg ? "true" : "false")
                   + ",\"hdr10Plus\":" + (hdr10plus ? "true" : "false") + "}";
        LOGDBG("[FbSettings|HdrChanged] Computed HDR flags: hdr10=%s dolbyVision=%s hlg=%s hdr10Plus=%s -> %s",
               hdr10 ? "true" : "false",
               dv ? "true" : "false",
               hlg ? "true" : "false",
               hdr10plus ? "true" : "false",
               jsonObject.c_str());
        return Core::ERROR_NONE;
    }

    // ---- Event exposure (Emit helpers) ----

    // PUBLIC_INTERFACE
    bool EmitOnVideoResolutionChanged()
    {
        std::string payload;
        if (GetVideoResolution(payload) != Core::ERROR_NONE) {
            LOGERR("[FbSettings|VideoResolutionChanged] handler=GetVideoResolution failed to compute payload");
            return false;
        }
        // Transform to rpcv2_event wrapper: { "videoResolution": $event_handler_response }
        const std::string wrapped = std::string("{\"videoResolution\":") + payload + "}";
        LOGINFO("[FbSettings|VideoResolutionChanged] Final rpcv2_event payload=%s", wrapped.c_str());
        if (ShouldEmitDebounced(EVENT_ON_VIDEO_RES_CHANGED, wrapped)) {
            LOGDBG("[FbSettings|VideoResolutionChanged] Emitting event: %s", EVENT_ON_VIDEO_RES_CHANGED);
            Dispatch(EVENT_ON_VIDEO_RES_CHANGED, wrapped);
            return true;
        }
        LOGDBG("[FbSettings|VideoResolutionChanged] Debounced/dropped");
        return false;
    }

    // PUBLIC_INTERFACE
    bool EmitOnScreenResolutionChanged()
    {
        std::string payload;
        if (GetScreenResolution(payload) != Core::ERROR_NONE) {
            LOGERR("[FbSettings|ScreenResolutionChanged] handler=GetScreenResolution failed to compute payload");
            return false;
        }
        // Transform to rpcv2_event wrapper: { "screenResolution": $event }
        const std::string wrapped = std::string("{\"screenResolution\":") + payload + "}";
        LOGINFO("[FbSettings|ScreenResolutionChanged] Final rpcv2_event payload=%s", wrapped.c_str());
        if (ShouldEmitDebounced(EVENT_ON_SCREEN_RES_CHANGED, wrapped)) {
            LOGDBG("[FbSettings|ScreenResolutionChanged] Emitting event: %s", EVENT_ON_SCREEN_RES_CHANGED);
            Dispatch(EVENT_ON_SCREEN_RES_CHANGED, wrapped);
            return true;
        }
        LOGDBG("[FbSettings|ScreenResolutionChanged] Debounced/dropped");
        return false;
    }

    // PUBLIC_INTERFACE
    bool EmitOnHdcpChanged()
    {
        std::string payload;
        if (GetHdcp(payload) != Core::ERROR_NONE) {
            LOGERR("[FbSettings|HdcpChanged] handler=GetHdcp failed to compute payload");
            return false;
        }
        LOGINFO("[FbSettings|HdcpChanged] Final rpcv2_event payload=%s", payload.c_str());
        if (ShouldEmitDebounced(EVENT_ON_HDCP_CHANGED, payload)) {
            LOGDBG("[FbSettings|HdcpChanged] Emitting event: %s", EVENT_ON_HDCP_CHANGED);
            Dispatch(EVENT_ON_HDCP_CHANGED, payload);
            return true;
        }
        LOGDBG("[FbSettings|HdcpChanged] Debounced/dropped");
        return false;
    }

    // PUBLIC_INTERFACE
    bool EmitOnHdrChanged()
    {
        std::string payload;
        if (GetHdr(payload) != Core::ERROR_NONE) {
            LOGERR("[FbSettings|HdrChanged] handler=GetHdr failed to compute payload");
            return false;
        }
        LOGINFO("[FbSettings|HdrChanged] Final rpcv2_event payload=%s", payload.c_str());
        if (ShouldEmitDebounced(EVENT_ON_HDR_CHANGED, payload)) {
            LOGDBG("[FbSettings|HdrChanged] Emitting event: %s", EVENT_ON_HDR_CHANGED);
            Dispatch(EVENT_ON_HDR_CHANGED, payload);
            return true;
        }
        LOGDBG("[FbSettings|HdrChanged] Debounced/dropped");
        return false;
    }

    // ---- AppNotifications registration hook ----
    // Called by SettingsDelegate when app subscribes/unsubscribes to events.
    bool HandleEvent(const std::string& event, const bool listen, bool& registrationError) override
    {
        registrationError = false;

        const std::string evLower = ToLower(event);

        // Supported events (case-insensitive)
        if (evLower == "device.onvideoresolutionchanged"
            || evLower == "device.onscreenresolutionchanged"
            || evLower == "device.onhdcpchanged"
            || evLower == "device.onhdrchanged")
        {
            LOGINFO("[FbSettings|EventRegistration] event=%s listen=%s", event.c_str(), listen ? "true" : "false");
            if (listen) {
                AddNotification(event);
                // Ensure underlying Thunder subscriptions are active
                SetupDisplaySettingsSubscription();
                SetupHdcpProfileSubscription();
                registrationError = true; // indicate handled without error
                return true;
            } else {
                RemoveNotification(event);
                registrationError = true;
                return true;
            }
        }
        return false;
    }

    bool ShouldEmitDebounced(const std::string& eventKey, const std::string& payload)
    {
#ifdef ENABLE_DEBOUNCE
        std::lock_guard<std::mutex> lock(_debounceMutex);
        auto& entry = _debounce[eventKey]; // default-constructed if not present
        auto now = std::chrono::steady_clock::now();
        if (payload == entry.lastPayload) {
            auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - entry.lastTime);
            const auto window = DebounceWindow();
            if (delta < window) {
                // Drop identical payload emitted within debounce window
                LOGDBG("SystemDelegate: dropping duplicate event %s within %lldms (window=%lldms)", eventKey.c_str(), (long long)delta.count(), (long long)window.count());
                return false;
            }
        }
        entry.lastPayload = payload;
        entry.lastTime = now;
        LOGDBG("SystemDelegate: debounced acceptance for event %s (payloadLen=%zu)", eventKey.c_str(), payload.size());
        return true;
#else
        (void)eventKey;
        (void)payload;
        return true; // Debounce disabled: always emit
#endif
    }

private:

    inline std::shared_ptr<WPEFramework::Utils::JSONRPCDirectLink> AcquireLink(const std::string& callsign) const
    {
        // Create a direct JSON-RPC link to the Thunder plugin using the Supporting_Files helper.
        if (_shell == nullptr)
        {
            LOGERR("SystemDelegate: shell is null");
            return nullptr;
        }
        return WPEFramework::Utils::GetThunderControllerClient(_shell, callsign);
    }

    inline std::shared_ptr<WPEFramework::Utils::JSONRPCDirectLink> AcquireLink() const
    {
        return AcquireLink(SYSTEM_CALLSIGN);
    }

    static std::string ToLower(const std::string &in)
    {
        std::string out;
        out.reserve(in.size());
        for (char c : in)
        {
            out.push_back(static_cast<char>(::tolower(static_cast<unsigned char>(c))));
        }
        return out;
    }

    static std::string TerritoryThunderToFirebolt(const std::string &terr, const std::string &deflt)
    {
        if (EqualsIgnoreCase(terr, "USA"))
            return "US";
        if (EqualsIgnoreCase(terr, "CAN"))
            return "CA";
        if (EqualsIgnoreCase(terr, "ITA"))
            return "IT";
        if (EqualsIgnoreCase(terr, "GBR"))
            return "GB";
        if (EqualsIgnoreCase(terr, "IRL"))
            return "IE";
        if (EqualsIgnoreCase(terr, "AUS"))
            return "AU";
        if (EqualsIgnoreCase(terr, "AUT"))
            return "AT";
        if (EqualsIgnoreCase(terr, "CHE"))
            return "CH";
        if (EqualsIgnoreCase(terr, "DEU"))
            return "DE";
        return deflt;
    }

    static std::string TerritoryFireboltToThunder(const std::string &code, const std::string &deflt)
    {
        if (EqualsIgnoreCase(code, "US"))
            return "USA";
        if (EqualsIgnoreCase(code, "CA"))
            return "CAN";
        if (EqualsIgnoreCase(code, "IT"))
            return "ITA";
        if (EqualsIgnoreCase(code, "GB"))
            return "GBR";
        if (EqualsIgnoreCase(code, "IE"))
            return "IRL";
        if (EqualsIgnoreCase(code, "AU"))
            return "AUS";
        if (EqualsIgnoreCase(code, "AT"))
            return "AUT";
        if (EqualsIgnoreCase(code, "CH"))
            return "CHE";
        if (EqualsIgnoreCase(code, "DE"))
            return "DEU";
        return deflt;
    }

    static bool EqualsIgnoreCase(const std::string &a, const std::string &b)
    {
        if (a.size() != b.size())
            return false;
        for (size_t i = 0; i < a.size(); ++i)
        {
            if (::tolower(static_cast<unsigned char>(a[i])) != ::tolower(static_cast<unsigned char>(b[i])))
            {
                return false;
            }
        }
        return true;
    }

    // Subscription helpers (stubs). These ensure symbols exist and can be expanded to real subscriptions.
    void SetupDisplaySettingsSubscription() {
        // TODO: Attach to DisplaySettings notifications if needed. No-op for build stability.
        LOGDBG("SystemDelegate: SetupDisplaySettingsSubscription (noop)");
    }

    void SetupHdcpProfileSubscription() {
        // TODO: Attach to HdcpProfile notifications if needed. No-op for build stability.
        LOGDBG("SystemDelegate: SetupHdcpProfileSubscription (noop)");
    }

private:
    PluginHost::IShell *_shell;
    std::unordered_set<std::string> _subscriptions;

#ifdef ENABLE_DEBOUNCE
    // Debounce tracking (only when enabled)
    struct DebounceEntry {
        std::chrono::steady_clock::time_point lastTime{};
        std::string lastPayload{};
    };

    std::mutex _debounceMutex;
    std::unordered_map<std::string, DebounceEntry> _debounce;
#endif
};

