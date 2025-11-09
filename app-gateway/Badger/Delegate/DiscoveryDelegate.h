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
         DiscoveryDelegate(PluginHost::IShell* shell) : _shell(shell) { LOGDBG("DiscoveryDelegate constructed (shell=%p)", static_cast<void*>(_shell)); }

         ~DiscoveryDelegate() = default;

       public:
         // PUBLIC_INTERFACE
         uint32_t MediaEventAccountLink(const std::string& appId, const std::string& payload, std::string& result) {
             /**
              * Routes badger.mediaEventAccountLink to org.rdk.FbDiscovery.* as follows:
              * - If event.completed == true -> call watchNext with identifiers.entityId = event.contentId
              * - Else -> call watched with { entityId=event.contentId, completed, progress, watchedOn? }
              * Progress is passed through without normalization.
              * Always returns {} result on success.
              */
             JsonObject payloadObj;
             if (!payload.empty()) {
                 payloadObj.FromString(payload);
             }

             JsonObject eventObj;
             if (payloadObj.HasLabel("event") && payloadObj["event"].Content() == Core::JSON::Variant::type::OBJECT) {
                 eventObj = payloadObj["event"].Object();
             }

             const std::string contentId = eventObj.HasLabel("contentId") ? eventObj["contentId"].String() : "";
             const bool completed = eventObj.HasLabel("completed") ? eventObj["completed"].Boolean() : false;

             if (contentId.empty()) {
                 LOGWARN("mediaEventAccountLink: missing event.contentId, returning {}");
                 result = "{}";
                 return Core::ERROR_NONE;
             }

             auto link = DelegateUtils::AcquireLink(_shell, FB_DISCOVERY_CALLSIGN);
             if (!link) {
                 LOGWARN("Discovery link unavailable, returning empty object");
                 result = "{}";
                 return Core::ERROR_UNAVAILABLE;
             }

             if (completed) {
                 // Map to FbDiscovery.watchNext (which internally maps to Watched with progress=1.0)
                 JsonObject identifiersObj;
                 identifiersObj["entityId"] = contentId;
                 std::string identifiersStr;
                 identifiersObj.ToString(identifiersStr);

                 JsonObject params;
                 JsonObject response;
                 JsonObject context;
                 context["appId"] = appId;
                 params["context"] = context;
                 params["identifiers"] = identifiersStr;

                 uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("watchNext"), params, response);
                 if (rc != Core::ERROR_NONE) {
                     LOGERR("watchNext RPC failed, rc=%u", rc);
                     result = "{}";
                     return rc;
                 }
                 result = "{}";
                 return Core::ERROR_NONE;
             } else {
                 // Route to FbDiscovery.watched with progress as-is (no normalization)
                 JsonObject params;
                 JsonObject response;
                 JsonObject context;
                 context["appId"] = appId;
                 params["context"] = context;

                 params["entityId"] = contentId;
                 if (eventObj.HasLabel("progress")) {
                     params["progress"] = eventObj["progress"];
                 }
                 // Completed flag should be forwarded as provided (false in this branch or passed value)
                 if (eventObj.HasLabel("completed")) {
                     params["completed"] = eventObj["completed"];
                 } else {
                     params["completed"] = Core::JSON::Boolean(false);
                 }
                 if (eventObj.HasLabel("watchedOn")) {
                     params["watchedOn"] = eventObj["watchedOn"];
                 }

                 uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("watched"), params, response);
                 if (rc != Core::ERROR_NONE) {
                     LOGERR("watched RPC failed, rc=%u", rc);
                     result = "{}";
                     return rc;
                 }
                 result = "{}";
                 return Core::ERROR_NONE;
             }
         }

         // PUBLIC_INTERFACE
         uint32_t ContentAccess(const std::string& appId, const std::string& idsPayload, std::string& result) {
             /**
              * Helper to call FbDiscovery.contentAccess with ids payload.
              * idsPayload should be a JSON string for: { "entitlements": [...], "availabilities": [...] } (either or both).
              * Always returns {} on success.
              */
             auto link = DelegateUtils::AcquireLink(_shell, FB_DISCOVERY_CALLSIGN);
             if (!link) {
                 LOGWARN("Discovery link unavailable, returning {}");
                 result = "{}";
                 return Core::ERROR_UNAVAILABLE;
             }

             JsonObject params;
             JsonObject response;
             JsonObject context;
             context["appId"] = appId;
             params["context"] = context;
             params["ids"] = idsPayload;

             uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("contentAccess"), params, response);
             if (rc != Core::ERROR_NONE) {
                 LOGERR("contentAccess RPC failed, rc=%u", rc);
                 result = "{}";
                 return rc;
             }
             result = "{}";
             return Core::ERROR_NONE;
         }

         // PUBLIC_INTERFACE
         uint32_t WatchNext(const std::string& appId, const std::string& contentId, std::string& result) {
             /**
              * Helper to call FbDiscovery.watchNext with identifiers.entityId = contentId.
              * Always returns {} on success.
              */
             auto link = DelegateUtils::AcquireLink(_shell, FB_DISCOVERY_CALLSIGN);
             if (!link) {
                 LOGWARN("Discovery link unavailable, returning {}");
                 result = "{}";
                 return Core::ERROR_UNAVAILABLE;
             }

             JsonObject identifiersObj;
             identifiersObj["entityId"] = contentId;
             std::string identifiersStr;
             identifiersObj.ToString(identifiersStr);

             JsonObject params;
             JsonObject response;
             JsonObject context;
             context["appId"] = appId;
             params["context"] = context;
             params["identifiers"] = identifiersStr;

             uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("watchNext"), params, response);
             if (rc != Core::ERROR_NONE) {
                 LOGERR("watchNext RPC failed, rc=%u", rc);
                 result = "{}";
                 return rc;
             }
             result = "{}";
             return Core::ERROR_NONE;
         }

         // PUBLIC_INTERFACE
         uint32_t Watched(const std::string& appId, const std::string& entityId, const double progress, const bool completed, const std::string& watchedOn, std::string& result) {
             /**
              * Helper to call FbDiscovery.watched with given parameters.
              * Always returns {} on success.
              */
             auto link = DelegateUtils::AcquireLink(_shell, FB_DISCOVERY_CALLSIGN);
             if (!link) {
                 LOGWARN("Discovery link unavailable, returning {}");
                 result = "{}";
                 return Core::ERROR_UNAVAILABLE;
             }

             JsonObject params;
             JsonObject response;
             JsonObject context;
             context["appId"] = appId;
             params["context"] = context;

             params["entityId"] = entityId;
             params["progress"] = Core::JSON::Double(progress);
             params["completed"] = Core::JSON::Boolean(completed);
             if (watchedOn.empty() == false) {
                 params["watchedOn"] = watchedOn;
             }

             uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("watched"), params, response);
             if (rc != Core::ERROR_NONE) {
                 LOGERR("watched RPC failed, rc=%u", rc);
                 result = "{}";
                 return rc;
             }
             result = "{}";
             return Core::ERROR_NONE;
         }

         // PUBLIC_INTERFACE
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

         // PUBLIC_INTERFACE
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
