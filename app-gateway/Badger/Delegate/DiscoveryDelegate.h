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
 #include <ctime>
 #include <cctype>
 #include <cstdlib>
 #include <string>
 #include <cstdint>

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
              *
              * This function normalizes:
              *  - entitlements: id -> entitlementId, startDate/endDate (epoch ms or s) -> startTime/endTime (ISO8601 UTC Z)
              *  - availabilities: startDate/endDate (epoch ms or s) -> startTime/endTime, preserves type/id/catalogId
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

             // Parse ids object from payload
             JsonObject idsObject;
             if (!idsPayload.empty()) {
                 idsObject.FromString(idsPayload);
             }

             // Normalize entitlements array if present
             if (idsObject.HasLabel("entitlements") && idsObject["entitlements"].Content() == Core::JSON::Variant::type::ARRAY) {
                 auto inArr = idsObject["entitlements"].Array();
                 Core::JSON::ArrayType<Core::JSON::Variant> outArr;

                 for (uint32_t i = 0; i < inArr.Length(); ++i) {
                     Core::JSON::Variant& v = inArr[i];
                     if (v.Content() != Core::JSON::Variant::type::OBJECT) {
                         continue;
                     }
                     JsonObject ent = v.Object();

                     // Start with a copy to preserve any extra fields
                     JsonObject normalized = ent;

                     // Map id -> entitlementId if entitlementId is not already present
                     if (!ent.HasLabel("entitlementId") && ent.HasLabel("id")) {
                         normalized["entitlementId"] = ent["id"];
                     }

                     // Normalize date fields to ISO8601 UTC Z if not already present
                     NormalizeDateField(normalized, "startDate", "startTime");
                     NormalizeDateField(normalized, "endDate", "endTime");

                     // Append normalized object to array
                     Core::JSON::Variant& outVar = outArr.Add();
                     outVar = normalized;
                 }

                 idsObject["entitlements"] = outArr;
             }

             // Normalize availabilities array if present
             if (idsObject.HasLabel("availabilities") && idsObject["availabilities"].Content() == Core::JSON::Variant::type::ARRAY) {
                 auto inArr = idsObject["availabilities"].Array();
                 Core::JSON::ArrayType<Core::JSON::Variant> outArr;

                 for (uint32_t i = 0; i < inArr.Length(); ++i) {
                     Core::JSON::Variant& v = inArr[i];
                     if (v.Content() != Core::JSON::Variant::type::OBJECT) {
                         continue;
                     }
                     JsonObject av = v.Object();

                     // Start with a copy to preserve all fields (type/id/catalogId/etc.)
                     JsonObject normalized = av;

                     // Normalize dates
                     NormalizeDateField(normalized, "startDate", "startTime");
                     NormalizeDateField(normalized, "endDate", "endTime");

                     Core::JSON::Variant& outVar = outArr.Add();
                     outVar = normalized;
                 }

                 idsObject["availabilities"] = outArr;
             }

             // Log the transformed payload for verification
             std::string normalizedIdsStr;
             idsObject.ToString(normalizedIdsStr);
             LOGINFO("ContentAccess normalized ids for appId=%s: %s", appId.c_str(), normalizedIdsStr.c_str());

             params["ids"] = normalizedIdsStr;

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
             Core::JSON::ArrayType<Core::JSON::Variant> entArr;
             if (!entitlements.empty()) {
                 entArr.FromString(entitlements);
             }
             params["entitlements"] = entArr;

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
         // Convert epoch (ms or s) Variant to milliseconds, if possible
         static bool TryEpochVariantToMillis(const Core::JSON::Variant& v, uint64_t& outMillis) {
             using Type = Core::JSON::Variant::type;
             if (v.Content() == Type::NUMBER) {
                 int64_t n = v.Number();
                 if (n < 0) return false;
                 uint64_t u = static_cast<uint64_t>(n);
                 // Heuristic: >= 1e12 -> ms, else seconds
                 outMillis = (u >= 100000000000ULL) ? u : (u * 1000ULL);
                 return true;
             }
             if (v.Content() == Type::STRING) {
                 std::string s = v.String();
                 // Trim whitespace
                 size_t start = 0, end = s.size();
                 while (start < end && std::isspace(static_cast<unsigned char>(s[start]))) start++;
                 while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) end--;
                 if (start >= end) return false;
                 uint64_t val = 0;
                 for (size_t i = start; i < end; ++i) {
                     if (!std::isdigit(static_cast<unsigned char>(s[i]))) {
                         return false;
                     }
                     val = (val * 10ULL) + static_cast<uint64_t>(s[i] - '0');
                 }
                 outMillis = ((end - start) >= 13) ? val : (val * 1000ULL);
                 return true;
             }
             return false;
         }

         // Format milliseconds since epoch to ISO8601 UTC with Z
         static std::string FormatIso8601FromMillis(uint64_t ms) {
             time_t sec = static_cast<time_t>(ms / 1000ULL);
             struct tm tmUtc;
 #if defined(_WIN32)
             gmtime_s(&tmUtc, &sec);
 #else
             gmtime_r(&sec, &tmUtc);
 #endif
             char buf[32] = {0};
             strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tmUtc);
             return std::string(buf);
         }

         // Normalize a date field on obj: if newKey not present, and oldKey present (ms or s), set newKey as ISO8601 Z
         static void NormalizeDateField(JsonObject& obj, const char* oldKey, const char* newKey) {
             // If already has normalized field, do nothing
             if (obj.HasLabel(newKey) && obj[newKey].Content() == Core::JSON::Variant::type::STRING) {
                 return;
             }
             if (obj.HasLabel(oldKey)) {
                 uint64_t ms = 0;
                 if (TryEpochVariantToMillis(obj[oldKey], ms)) {
                     obj[newKey] = FormatIso8601FromMillis(ms);
                 }
             }
         }

       private:
         PluginHost::IShell* _shell;
     };
 }  // namespace WPEFramework
