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
#include "UtilsLogging.h"

#ifndef FB_DISCOVERY_CALLSIGN
#define FB_DISCOVERY_CALLSIGN "org.rdk.FbDiscovery"
#endif

namespace WPEFramework {
    class DiscoveryDelegate {
      public:
        DiscoveryDelegate(PluginHost::IShell* shell) : _shell(shell) {}

        ~DiscoveryDelegate() = default;

      public:
        uint32_t MediaEventAccountLink(const std::string& appId, const std::string& payload, std::string& result) {
            auto link = DelegateUtils::AcquireLink(_shell, FB_DISCOVERY_CALLSIGN);
            if (!link) {
                LOGWARN("Advertising link unavailable, returning empty object");
                result = "{}";
                return Core::ERROR_UNAVAILABLE;
            }

            JsonObject params;
            JsonObject response;
            JsonObject context;
            context["appId"] = appId;
            params["context"] = context;

            JsonObject payloadObj;
            if (!payload.empty()) {
                payloadObj.FromString(payload);
            }

            // Safely copy fields (use empty/default if missing)
            params["entityId"] = payloadObj.HasLabel("entityId") ? payloadObj["entityId"] : JsonValue();
            params["progress"] = payloadObj.HasLabel("progress") ? payloadObj["progress"] : JsonValue();
            params["completed"] = payloadObj.HasLabel("completed") ? payloadObj["completed"] : JsonValue();
            params["watchedOn"] = payloadObj.HasLabel("watchedOn") ? payloadObj["watchedOn"] : JsonValue();

            uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("watched"), params, response);
            if (rc != Core::ERROR_NONE) {
                LOGERR("mediaEventAccountLink RPC failed, rc=%u", rc);
                result = "{}";
                return rc;
            }
            response.ToString(result);
            LOGINFO("mediaEventAccountLink JSON: %s", result.c_str());
            return Core::ERROR_NONE;
        }

        uint32_t SignIn(const std::string& appId, const std::string& entitlements, bool& success) {
            auto link = DelegateUtils::AcquireLink(_shell, FB_DISCOVERY_CALLSIGN);
            if (!link) {
                LOGWARN("Advertising link unavailable, returning failure");
                success = false;
                return Core::ERROR_UNAVAILABLE;
            }

            JsonObject params;
            JsonObject response;
            JsonObject context;
            context["appId"] = appId;
            params["context"] = context;
            params["entitlements"] = entitlements;

            uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("signIn"), params, response);
            if (rc != Core::ERROR_NONE) {
                LOGERR("SignIn RPC failed, rc=%u", rc);
                success = false;
                return rc;
            }

            success = response.HasLabel("success") ? response["success"].Boolean() : false;

            LOGINFO("SignIn success: %s", success ? "true" : "false");
            return Core::ERROR_NONE;
        }

        uint32_t SignOut(const std::string& appId, bool& success) {
            auto link = DelegateUtils::AcquireLink(_shell, FB_DISCOVERY_CALLSIGN);
            if (!link) {
                LOGWARN("Advertising link unavailable, returning failure");
                success = false;
                return Core::ERROR_UNAVAILABLE;
            }

            JsonObject params;
            JsonObject response;
            JsonObject context;
            context["appId"] = appId;
            params["context"] = context;

            uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("signOut"), params, response);
            if (rc != Core::ERROR_NONE) {
                LOGERR("SignOut RPC failed, rc=%u", rc);
                success = false;
                return rc;
            }

            success = response.HasLabel("success") ? response["success"].Boolean() : false;

            LOGINFO("SignOut success: %s", success ? "true" : "false");
            return Core::ERROR_NONE;
        }

        uint32_t EntitlementsAccountLink(const std::string& appId, const std::string& payload, std::string& result){
            JsonObject payloadObj;
            if (!payload.empty()) {
                payloadObj.FromString(payload);
            }

            std::string type = payloadObj.HasLabel("type") ? payloadObj["type"].String() : "";
            std::string action = payloadObj.HasLabel("action") ? payloadObj["action"].String() : "";

            if (type == "accountLink") {
                if (action == "signIn") {
                    bool success = false;
                    uint32_t rc = SignIn(appId, payload, success);
                    if (rc != Core::ERROR_NONE) {
                        return rc;
                    }
                    JsonObject response;
                    response["success"] = success;
                    response.ToString(result);
                    return Core::ERROR_NONE;
                } else if (action == "signOut") {
                    bool success = false;
                    uint32_t rc = SignOut(appId, success);
                    if (rc != Core::ERROR_NONE) {
                        return rc;
                    }
                    JsonObject response;
                    response["success"] = success;
                    response.ToString(result);
                    return Core::ERROR_NONE;
                } else {
                    LOGERR("Unsupported action for accountLink: %s", action.c_str());
                    return Core::ERROR_NOT_SUPPORTED;
                }
            } else {
                LOGERR("Unsupported type for EntitlementsAccountLink: %s", type.c_str());
                return Core::ERROR_NOT_SUPPORTED;
            }
        }

      private:
        PluginHost::IShell* _shell;
    };
}  // namespace WPEFramework