#include "TokenClient.h"

#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

    bool TokenClient::GetPlatformToken(const std::string& appId,
                                       const std::string& xact,
                                       const std::string& sat,
                                       std::string& outTokenJson,
                                       std::string& errMsg)
    {
        LOGINFO("TokenClient::GetPlatformToken called (appId='%s')", appId.c_str());
        VARIABLE_IS_NOT_USED std::string _xact = xact;
        VARIABLE_IS_NOT_USED std::string _sat  = sat;

        // TODO: Wire to ott_token.proto generated stubs when available.
        outTokenJson.clear();
        errMsg = "Not implemented yet";
        return false;
    }

    bool TokenClient::GetAuthToken(const std::string& appId,
                                   const std::string& sat,
                                   std::string& outTokenJson,
                                   std::string& errMsg)
    {
        LOGINFO("TokenClient::GetAuthToken called (appId='%s')", appId.c_str());
        VARIABLE_IS_NOT_USED std::string _sat  = sat;

        // TODO: Wire to ott_token.proto generated stubs when available.
        outTokenJson.clear();
        errMsg = "Not implemented yet";
        return false;
    }

} // namespace Plugin
} // namespace WPEFramework
