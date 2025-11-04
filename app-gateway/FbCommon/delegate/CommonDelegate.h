#pragma once

#include "../Module.h"
#include "UtilsLogging.h"
#include "UtilsJsonrpcDirectLink.h"
#include <core/JSON.h>

namespace WPEFramework {
namespace Plugin {

/**
 * CommonDelegate
 * Minimal delegate used by FbCommonImplementation to fetch device info via Thunder
 * plugins (DisplaySettings, HdcpProfile, System). Where not available, returns
 * reasonable placeholders.
 */
class CommonDelegate {
public:
    explicit CommonDelegate(PluginHost::IShell* shell)
        : _shell(shell) {}

    ~CommonDelegate() = default;

    // PUBLIC_INTERFACE
    Core::hresult GetCurrentResolution(std::string& outJson) {
        /** Get [w, h] screen resolution via DisplaySettings.getCurrentResolution or fallback. */
        outJson = "[1920,1080]"; // default fallback
        if (_shell == nullptr) {
            return Core::ERROR_UNAVAILABLE;
        }
        auto link = Utils::GetThunderControllerClient(_shell, "org.rdk.DisplaySettings");
        if (!link) return Core::ERROR_UNAVAILABLE;

        std::string response;
        Core::hresult rc = link->Invoke<std::string, std::string>("getCurrentResolution", "{}", response);
        if (rc != Core::ERROR_NONE || response.empty()) {
            return Core::ERROR_GENERAL;
        }

        // Try to parse {"result":{"w":..., "h":...}} and render as [w,h]
        Core::JSON::VariantContainer obj;
        Core::OptionalType<Core::JSON::Error> error;
        if (obj.FromString(response, error)) {
            auto r = obj.Get(_T("result"));
            if (r.Content() == Core::JSON::Variant::type::OBJECT) {
                auto wv = r.Object().Get(_T("w"));
                auto hv = r.Object().Get(_T("h"));
                if (wv.Content() == Core::JSON::Variant::type::NUMBER && hv.Content() == Core::JSON::Variant::type::NUMBER) {
                    outJson = "[" + std::to_string(static_cast<int>(wv.Number())) + "," + std::to_string(static_cast<int>(hv.Number())) + "]";
                    return Core::ERROR_NONE;
                }
            }
        }
        return Core::ERROR_NONE; // keep fallback
    }

    // PUBLIC_INTERFACE
    Core::hresult GetHdcpStatus(std::string& outJson) {
        /** Get HDCP status via HdcpProfile.getHDCPStatus. Render {"hdcp1.4":bool,"hdcp2.2":bool}. */
        outJson = "{\"hdcp1.4\":false,\"hdcp2.2\":false}";
        if (_shell == nullptr) {
            return Core::ERROR_UNAVAILABLE;
        }
        auto link = Utils::GetThunderControllerClient(_shell, "org.rdk.HdcpProfile");
        if (!link) return Core::ERROR_UNAVAILABLE;

        std::string response;
        Core::hresult rc = link->Invoke<std::string, std::string>("getHDCPStatus", "{}", response);
        if (rc != Core::ERROR_NONE || response.empty()) {
            return Core::ERROR_GENERAL;
        }

        // Parse version/reason
        bool hdcp14 = false;
        bool hdcp22 = false;
        Core::JSON::VariantContainer obj;
        Core::OptionalType<Core::JSON::Error> error;
        if (obj.FromString(response, error)) {
            auto r = obj.Get(_T("result"));
            if (r.Content() == Core::JSON::Variant::type::OBJECT) {
                auto succ = r.Object().Get(_T("success"));
                auto status = r.Object().Get(_T("HDCPStatus"));
                if (succ.Content() == Core::JSON::Variant::type::BOOLEAN && succ.Boolean() &&
                    status.Content() == Core::JSON::Variant::type::OBJECT) {
                    auto reason = status.Object().Get(_T("hdcpReason"));
                    auto version = status.Object().Get(_T("currentHDCPVersion"));
                    if (reason.Content() == Core::JSON::Variant::type::NUMBER && static_cast<int>(reason.Number()) == 2 &&
                        version.Content() == Core::JSON::Variant::type::STRING) {
                        const string v = version.String();
                        if (v == "1.4") { hdcp14 = true; }
                        else { hdcp22 = true; }
                    }
                }
            }
        }
        outJson = std::string("{\"hdcp1.4\":") + (hdcp14 ? "true" : "false")
                + ",\"hdcp2.2\":" + (hdcp22 ? "true" : "false") + "}";
        return Core::ERROR_NONE;
    }

    // PUBLIC_INTERFACE
    Core::hresult GetDeviceName(std::string& name) {
        /** Get friendly name via System.getFriendlyName, fallback to "Living Room". */
        name = "Living Room";
        if (_shell == nullptr) return Core::ERROR_UNAVAILABLE;

        auto link = Utils::GetThunderControllerClient(_shell, "org.rdk.System");
        if (!link) return Core::ERROR_UNAVAILABLE;

        std::string response;
        if (link->Invoke<std::string, std::string>("getFriendlyName", "{}", response) != Core::ERROR_NONE) {
            return Core::ERROR_GENERAL;
        }
        Core::JSON::VariantContainer obj;
        Core::OptionalType<Core::JSON::Error> error;
        if (obj.FromString(response, error)) {
            auto r = obj.Get(_T("result"));
            if (r.Content() == Core::JSON::Variant::type::OBJECT) {
                auto fn = r.Object().Get(_T("friendlyName"));
                if (fn.Content() == Core::JSON::Variant::type::STRING && fn.String().empty() == false) {
                    name = fn.String();
                    return Core::ERROR_NONE;
                }
            }
        }
        return Core::ERROR_NONE;
    }

    // PUBLIC_INTERFACE
    Core::hresult SetDeviceName(const std::string& name) {
        /** Set friendly name via System.setFriendlyName({friendlyName:...}). */
        if (_shell == nullptr) return Core::ERROR_UNAVAILABLE;
        auto link = Utils::GetThunderControllerClient(_shell, "org.rdk.System");
        if (!link) return Core::ERROR_UNAVAILABLE;

        // Build params safely without assigning JSON::String into a Variant (which may trigger NumberType templates).
        // Serialize the Core::JSON::String and embed it directly in the JSON object text.
        Core::JSON::String jn;
        jn = name;
        string jsonName, params;
        jn.ToString(jsonName); // jsonName contains proper JSON-escaped string with quotes
        params = string("{\"friendlyName\":") + jsonName + "}";

        std::string response;
        return link->Invoke<std::string, std::string>("setFriendlyName", params, response);
    }

    // PUBLIC_INTERFACE
    Core::hresult GetDeviceMake(std::string& make) {
        /** Get device make via System.getDeviceInfo, fallback to "unknown". */
        make = "unknown";
        if (_shell == nullptr) return Core::ERROR_UNAVAILABLE;

        auto link = Utils::GetThunderControllerClient(_shell, "org.rdk.System");
        if (!link) return Core::ERROR_UNAVAILABLE;

        std::string response;
        if (link->Invoke<std::string, std::string>("getDeviceInfo", "{}", response) != Core::ERROR_NONE) {
            return Core::ERROR_GENERAL;
        }
        Core::JSON::VariantContainer obj;
        Core::OptionalType<Core::JSON::Error> error;
        if (obj.FromString(response, error)) {
            auto r = obj.Get(_T("result"));
            if (r.Content() == Core::JSON::Variant::type::OBJECT) {
                auto m = r.Object().Get(_T("make"));
                if (m.Content() == Core::JSON::Variant::type::STRING && m.String().empty() == false) {
                    make = m.String();
                    return Core::ERROR_NONE;
                }
            }
        }
        return Core::ERROR_NONE;
    }

    // PUBLIC_INTERFACE
    Core::hresult GetDeviceSku(std::string& sku) {
        /** Get SKU via System.getSystemVersions, mapping result.stbVersion split('_')[0]. */
        sku = "unknown";
        if (_shell == nullptr) return Core::ERROR_UNAVAILABLE;
        auto link = Utils::GetThunderControllerClient(_shell, "org.rdk.System");
        if (!link) return Core::ERROR_UNAVAILABLE;

        std::string response;
        if (link->Invoke<std::string, std::string>("getSystemVersions", "{}", response) != Core::ERROR_NONE) {
            return Core::ERROR_GENERAL;
        }
        Core::JSON::VariantContainer obj;
        Core::OptionalType<Core::JSON::Error> error;
        if (obj.FromString(response, error)) {
            auto r = obj.Get(_T("result"));
            if (r.Content() == Core::JSON::Variant::type::OBJECT) {
                auto v = r.Object().Get(_T("stbVersion"));
                if (v.Content() == Core::JSON::Variant::type::STRING && v.String().empty() == false) {
                    const string full = v.String();
                    const auto pos = full.find('_');
                    sku = (pos == string::npos) ? full : full.substr(0, pos);
                    return Core::ERROR_NONE;
                }
            }
        }
        return Core::ERROR_NONE;
    }

private:
    PluginHost::IShell* _shell;
};

} // namespace Plugin
} // namespace WPEFramework
