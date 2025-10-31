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

 #include <memory>
 #include <string>
 #include <unordered_set>

 #include <plugins/plugins.h>
 #include <core/JSON.h>
 #include "UtilsLogging.h"
 #include "UtilsJsonrpcDirectLink.h"

 // Define callsign constants
 #ifndef SYSTEM_CALLSIGN
 #define SYSTEM_CALLSIGN "org.rdk.System"
 #endif

 #ifndef DISPLAYSETTINGS_CALLSIGN
 #define DISPLAYSETTINGS_CALLSIGN "org.rdk.DisplaySettings"
 #endif

 #ifndef HDCPPROFILE_CALLSIGN
 #define HDCPPROFILE_CALLSIGN "org.rdk.HdcpProfile"
 #endif

 class SystemDelegate
 {
 public:
     SystemDelegate(PluginHost::IShell *shell)
         : _shell(shell)
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
         const uint32_t rc = link->Invoke<decltype(params), decltype(response)>("getDeviceInfo", params, response);
         if (rc == Core::ERROR_NONE)
         {
             // Accept either top-level or nested "result"
             if (response.HasLabel(_T("make")))
             {
                 make = response[_T("make")].String();
             }
             else if (response.HasLabel(_T("result")))
             {
                 auto r = response.Get(_T("result"));
                 if (r.Content() == WPEFramework::Core::JSON::Variant::type::OBJECT) {
                     auto m = r.Object().Get(_T("make"));
                     if (m.Content() == WPEFramework::Core::JSON::Variant::type::STRING) {
                         make = m.String();
                     }
                 }
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
         const uint32_t rc = link->Invoke<decltype(params), decltype(response)>("getFriendlyName", params, response);
         if (rc == Core::ERROR_NONE) {
             if (response.HasLabel(_T("friendlyName")))
             {
                 name = response[_T("friendlyName")].String();
             }
             else if (response.HasLabel(_T("result")))
             {
                 auto r = response.Get(_T("result"));
                 if (r.Content() == WPEFramework::Core::JSON::Variant::type::OBJECT) {
                     auto fn = r.Object().Get(_T("friendlyName"));
                     if (fn.Content() == WPEFramework::Core::JSON::Variant::type::STRING) {
                         name = fn.String();
                     }
                 }
             }
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
         const uint32_t rc = link->Invoke<decltype(params), decltype(response)>("setFriendlyName", params, response);
         if (rc == Core::ERROR_NONE) {
             bool success = false;
             if (response.HasLabel(_T("success"))) {
                 success = response[_T("success")].Boolean();
             } else if (response.HasLabel(_T("result"))) {
                 auto r = response.Get(_T("result"));
                 if (r.Content() == WPEFramework::Core::JSON::Variant::type::OBJECT) {
                     auto s = r.Object().Get(_T("success"));
                     success = (s.Content() == WPEFramework::Core::JSON::Variant::type::BOOLEAN) ? s.Boolean() : false;
                 }
             }
             if (success) {
                 return Core::ERROR_NONE;
             }
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
         const uint32_t rc = link->Invoke<decltype(params), decltype(response)>("getSystemVersions", params, response);
         if (rc != Core::ERROR_NONE)
         {
             LOGERR("SystemDelegate: getSystemVersions failed rc=%u", rc);
             return Core::ERROR_UNAVAILABLE;
         }

         std::string stbVersion;
         if (response.HasLabel(_T("stbVersion"))) {
             stbVersion = response[_T("stbVersion")].String();
         } else if (response.HasLabel(_T("result"))) {
             auto r = response.Get(_T("result"));
             if (r.Content() == WPEFramework::Core::JSON::Variant::type::OBJECT) {
                 auto sv = r.Object().Get(_T("stbVersion"));
                 if (sv.Content() == WPEFramework::Core::JSON::Variant::type::STRING) {
                     stbVersion = sv.String();
                 }
             }
         }

         if (stbVersion.empty())
         {
             LOGERR("SystemDelegate: getSystemVersions missing stbVersion");
             return Core::ERROR_UNAVAILABLE;
         }

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
         const uint32_t rc = link->Invoke<decltype(params), decltype(response)>("getTerritory", params, response);
         if (rc == Core::ERROR_NONE) {
             std::string terr;
             if (response.HasLabel(_T("territory"))) {
                 terr = response[_T("territory")].String();
             } else if (response.HasLabel(_T("result"))) {
                 auto r = response.Get(_T("result"));
                 if (r.Content() == WPEFramework::Core::JSON::Variant::type::OBJECT) {
                     auto t = r.Object().Get(_T("territory"));
                     if (t.Content() == WPEFramework::Core::JSON::Variant::type::STRING) {
                         terr = t.String();
                     }
                 }
             }
             if (!terr.empty()) {
                 code = TerritoryThunderToFirebolt(terr, "US");
             }
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
         const uint32_t rc = link->Invoke<decltype(params), decltype(response)>("setTerritory", params, response);
         if (rc == Core::ERROR_NONE) {
             bool success = false;
             if (response.HasLabel(_T("success"))) {
                 success = response[_T("success")].Boolean();
             } else if (response.HasLabel(_T("result"))) {
                 auto r = response.Get(_T("result"));
                 if (r.Content() == WPEFramework::Core::JSON::Variant::type::OBJECT) {
                     auto s = r.Object().Get(_T("success"));
                     success = (s.Content() == WPEFramework::Core::JSON::Variant::type::BOOLEAN) ? s.Boolean() : false;
                 }
             }
             if (success) {
                 return Core::ERROR_NONE;
             }
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
         const uint32_t rc = link->Invoke<decltype(params), decltype(response)>("getTimeZoneDST", params, response);
         if (rc == Core::ERROR_NONE)
         {
             bool success = false;
             if (response.HasLabel(_T("success"))) {
                 success = response[_T("success")].Boolean();
                 if (response.HasLabel(_T("timeZone"))) {
                     tz = response[_T("timeZone")].String();
                     return Core::ERROR_NONE;
                 }
             } else if (response.HasLabel(_T("result"))) {
                 auto r = response.Get(_T("result"));
                 if (r.Content() == WPEFramework::Core::JSON::Variant::type::OBJECT) {
                     auto s = r.Object().Get(_T("success"));
                     success = (s.Content() == WPEFramework::Core::JSON::Variant::type::BOOLEAN) ? s.Boolean() : false;
                     if (success) {
                         auto tzv = r.Object().Get(_T("timeZone"));
                         if (tzv.Content() == WPEFramework::Core::JSON::Variant::type::STRING) {
                             tz = tzv.String();
                             return Core::ERROR_NONE;
                         }
                     }
                 }
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
         const uint32_t rc = link->Invoke<decltype(params), decltype(response)>("setTimeZoneDST", params, response);
         if (rc == Core::ERROR_NONE) {
             bool success = false;
             if (response.HasLabel(_T("success"))) {
                 success = response[_T("success")].Boolean();
             } else if (response.HasLabel(_T("result"))) {
                 auto r = response.Get(_T("result"));
                 if (r.Content() == WPEFramework::Core::JSON::Variant::type::OBJECT) {
                     auto s = r.Object().Get(_T("success"));
                     success = (s.Content() == WPEFramework::Core::JSON::Variant::type::BOOLEAN) ? s.Boolean() : false;
                 }
             }
             if (success) {
                 return Core::ERROR_NONE;
             }
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

         std::string response;
         Core::hresult rc = link->Invoke<std::string, std::string>("getCurrentResolution", "{}", response);
         if (rc != Core::ERROR_NONE || response.empty()) {
             return Core::ERROR_GENERAL;
         }

         int w = 1920, h = 1080;
         WPEFramework::Core::JSON::VariantContainer obj;
         WPEFramework::Core::OptionalType<WPEFramework::Core::JSON::Error> error;
         if (obj.FromString(response, error)) {
             // Try top-level first
             auto wv = obj.Get(_T("w"));
             auto hv = obj.Get(_T("h"));
             if (wv.Content() == WPEFramework::Core::JSON::Variant::type::NUMBER &&
                 hv.Content() == WPEFramework::Core::JSON::Variant::type::NUMBER) {
                 w = static_cast<int>(wv.Number());
                 h = static_cast<int>(hv.Number());
             } else {
                 // Try nested result
                 auto r = obj.Get(_T("result"));
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
         }
         jsonArray = "[" + std::to_string(w) + "," + std::to_string(h) + "]";
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
                 }
             } catch (...) {
                 // keep defaults
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

         std::string response;
         Core::hresult rc = link->Invoke<std::string, std::string>("getHDCPStatus", "{}", response);
         if (rc != Core::ERROR_NONE || response.empty()) {
             return Core::ERROR_GENERAL;
         }

         bool hdcp14 = false;
         bool hdcp22 = false;

         WPEFramework::Core::JSON::VariantContainer obj;
         WPEFramework::Core::OptionalType<WPEFramework::Core::JSON::Error> error;
         if (obj.FromString(response, error)) {
             auto r = obj.Get(_T("result"));
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
         }

         jsonObject = std::string("{\"hdcp1.4\":") + (hdcp14 ? "true" : "false")
                    + ",\"hdcp2.2\":" + (hdcp22 ? "true" : "false") + "}";
         return Core::ERROR_NONE;
     }

     // PUBLIC_INTERFACE
     Core::hresult GetHdr(std::string &jsonObject)
     {
         /**
          * Stubbed HDR capability/state report.
          * Ideally obtained via HDMI status endpoint (/system/hdmi) or platform plugin.
          * Returns all-false by default to maintain interface contract.
          */
         jsonObject = "{\"hdr10\":false,\"dolbyVision\":false,\"hlg\":false,\"hdr10Plus\":false}";
         // Placeholder: no platform call implemented here to keep scope minimal.
         return Core::ERROR_NONE;
     }

 private:
     inline std::shared_ptr<WPEFramework::Utils::JSONRPCDirectLink> AcquireLink() const
     {
         // Create a direct JSON-RPC link to the Thunder System plugin using the Supporting_Files helper.
         if (_shell == nullptr)
         {
             LOGERR("SystemDelegate: shell is null");
             return nullptr;
         }
         return WPEFramework::Utils::GetThunderControllerClient(_shell, SYSTEM_CALLSIGN);
     }

     inline std::shared_ptr<WPEFramework::Utils::JSONRPCDirectLink> AcquireLink(const std::string& callsign) const
     {
         if (_shell == nullptr)
         {
             LOGERR("SystemDelegate: shell is null");
             return nullptr;
         }
         return WPEFramework::Utils::GetThunderControllerClient(_shell, callsign);
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

 private:
     PluginHost::IShell *_shell;
     std::unordered_set<std::string> _subscriptions;
 };
