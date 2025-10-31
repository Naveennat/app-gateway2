#include "FbCommonImplementation.h"
#include <core/JSON.h>
#include <sstream>

namespace WPEFramework {
namespace Plugin {

SERVICE_REGISTRATION(FbCommonImplementation, 1, 0, 0);

FbCommonImplementation::FbCommonImplementation()
    : mShell(nullptr)
{
}

FbCommonImplementation::~FbCommonImplementation()
{
    if (mShell != nullptr) {
        mShell->Release();
        mShell = nullptr;
    }
}

uint32_t FbCommonImplementation::Configure(PluginHost::IShell* shell)
{
    LOGINFO("Configuring FbCommon");
    ASSERT(shell != nullptr);
    mShell = shell;
    mShell->AddRef();
    mDelegate = std::make_shared<CommonDelegate>(mShell);
    return Core::ERROR_NONE;
}

Core::hresult FbCommonImplementation::HandleAppGatewayRequest(
    const Exchange::GatewayContext& /*context*/,
    const string& method,
    const string& payload,
    string& result)
{
    const std::string m = ToLower(method);
    LOGINFO("FbCommon HandleAppGatewayRequest: method=%s payload=%s", m.c_str(), payload.c_str());

    if (!mDelegate) {
        result = "{\"error\":\"FbCommon not configured\"}";
        return Core::ERROR_UNAVAILABLE;
    }

    // Device.screenResolution
    if (m == "device.screenresolution") {
        return mDelegate->GetCurrentResolution(result);
    }

    // Device.videoResolution (approximate using same resolution or fallback)
    if (m == "device.videoresolution") {
        // Reuse screen resolution as a proxy for video resolution
        Core::hresult rc = mDelegate->GetCurrentResolution(result);
        if (rc != Core::ERROR_NONE || result.empty()) {
            result = "[1920,1080]";
        }
        return Core::ERROR_NONE;
    }

    // Device.hdcp
    if (m == "device.hdcp") {
        return mDelegate->GetHdcpStatus(result);
    }

    // Device.hdr (placeholder when not available via /system/hdmi endpoint)
    if (m == "device.hdr") {
        // Placeholder per ripple rules fallback
        result = "{\"hdr10\":false,\"dolbyVision\":false,\"hlg\":false,\"hdr10Plus\":false}";
        return Core::ERROR_NONE;
    }

    // Device.audio (placeholder; in ripple this is sourced from /audio/setting/HDMIAudioFormat)
    if (m == "device.audio") {
        result = "{\"stereo\":true,\"dolbyAtmos\":false,\"dolbyDigital5.1\":false,\"dolbyDigital5.1+\":false}";
        return Core::ERROR_NONE;
    }

    // Device.make
    if (m == "device.make") {
        std::string make;
        Core::hresult rc = mDelegate->GetDeviceMake(make);
        result = make;
        return rc;
    }

    // Device.sku
    if (m == "device.sku") {
        std::string sku;
        Core::hresult rc = mDelegate->GetDeviceSku(sku);
        result = sku;
        return rc;
    }

    // Device.name
    if (m == "device.name") {
        std::string name;
        Core::hresult rc = mDelegate->GetDeviceName(name);
        result = name;
        return rc;
    }

    // Device.setName
    if (m == "device.setname") {
        Core::JSON::VariantContainer params;
        Core::OptionalType<Core::JSON::Error> error;
        string nameValue;
        if (params.FromString(payload, error)) {
            auto v = params.Get(_T("value"));
            if (v.Content() == Core::JSON::Variant::type::STRING) {
                nameValue = v.String();
            }
        }
        if (nameValue.empty()) {
            result = "{\"error\":\"Invalid payload\"}";
            return Core::ERROR_BAD_REQUEST;
        }
        Core::hresult rc = mDelegate->SetDeviceName(nameValue);
        if (rc == Core::ERROR_NONE) {
            result = "null";
        } else {
            result = "{\"error\":\"couldn't set name\"}";
        }
        return rc;
    }

    // Unknown method
    result = "{\"error\":\"Method not found\"}";
    LOGERR("FbCommon unsupported method: %s", method.c_str());
    return Core::ERROR_UNKNOWN_KEY;
}

} // namespace Plugin
} // namespace WPEFramework
