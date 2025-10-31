#pragma once

#include "Module.h"
#include <interfaces/IConfiguration.h>
#include <interfaces/IAppGateway.h>
#include <mutex>
#include "UtilsLogging.h"
#include "delegate/CommonDelegate.h"

namespace WPEFramework {
namespace Plugin {

class FbCommonImplementation : public Exchange::IAppGatewayRequestHandler, public Exchange::IConfiguration {
private:
    FbCommonImplementation(const FbCommonImplementation&) = delete;
    FbCommonImplementation& operator=(const FbCommonImplementation&) = delete;

public:
    FbCommonImplementation();
    ~FbCommonImplementation() override;

    BEGIN_INTERFACE_MAP(FbCommonImplementation)
        INTERFACE_ENTRY(Exchange::IAppGatewayRequestHandler)
        INTERFACE_ENTRY(Exchange::IConfiguration)
    END_INTERFACE_MAP

    // PUBLIC_INTERFACE
    Core::hresult HandleAppGatewayRequest(const Exchange::GatewayContext& context /* @in */,
                                          const string& method /* @in */,
                                          const string& payload /* @in */,
                                          string& result /* @out */) override;
    /** HandleAppGatewayRequest
     *  Routes specific Device.* methods to underlying RDK plugins (DisplaySettings, HdcpProfile, System)
     *  via CommonDelegate or returns placeholders, as appropriate.
     *  @param context Request context (not used in this minimal implementation).
     *  @param method Fully qualified Firebolt method name (e.g., "device.screenResolution").
     *  @param payload JSON string payload (e.g., for device.setName => { "value": "<name>" }).
     *  @param result Output JSON or primitive string response.
     */

    // IConfiguration interface
    uint32_t Configure(PluginHost::IShell* shell) override;

private:
    static std::string ToLower(const std::string& s) {
        std::string out = s;
        for (auto& c : out) c = static_cast<char>(::tolower(c));
        return out;
    }

    PluginHost::IShell* mShell;
    std::shared_ptr<CommonDelegate> mDelegate;
};

} // namespace Plugin
} // namespace WPEFramework
