/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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

#include "DeviceBridgeUtils.h"

#include "../helpers/WebSockets/SimpleWSClient.h"
#include "../helpers/utils.h"

#include <curl/curl.h>

#include <core/JSON.h>

using namespace std;

namespace WPEFramework {
namespace Plugin {

// Callback to handle CURL response
static size_t writeCurlResponse(void *ptr, size_t size, size_t nmemb, string *stream) {
    size_t realSize = size * nmemb;
    stream->append(static_cast<const char *>(ptr), realSize);
    return realSize;
}

// Helper function to perform a request via CURL and return response as a string.
string getCurlResponse(const string &url, const string &requestData = "") {
    string response;

    CURL *curl = curl_easy_init();
    if (!curl) {
        LOGERR("Failed to initialize CURL");
        return response;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "http");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCurlResponse);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    if (!requestData.empty()) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestData.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, requestData.length());
    } else {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    }

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        LOGERR("CURL operation failed: %s", curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);

    return response;
}

const std::string getHostForegroupAppID() {
    LOGINFO();

    WebSockets::SimpleWSClient client;
    const std::string serverUri = "ws://0:9005/as/apps/status";

    if (!client.connectAndSendRequest(serverUri)) {
        LOGERR("Failed to connect to server at URI: %s", serverUri.c_str());
        return "";
    }

    std::string response = client.getResponse();
    if (response.empty()) {
        LOGERR("No response received from server!");
        return "";
    }

    JsonObject joResponse(response);
    if (joResponse.HasLabel("apps")) {
        JsonArray apps = joResponse["apps"].Array();
        int len = apps.Length();

        if (len > 0) {
            auto j_itr = apps.Elements();
            j_itr.Reset();
            for (int i = 0; i < len && j_itr.Next(); i++) {
                JsonObject app = j_itr.Current().Object();
                // Updated following LLDEV-40627: Adjusted logic to use "status" == "RUNNING" for foreground app detection. 
                // Initial "fireboltStatus" check disabled due to issues in test phase.
                // if (app.HasLabel("fireboltStatus") && app["fireboltStatus"].Value() == "FOREGROUND") {
                    if (app.HasLabel("status") && app["status"].Value() == "VISIBLE") {
                        if (app.HasLabel("appId")) {
                            return app["appId"].Value();
                        }
                    }
                // }
            }
        }
    }

    LOGINFO("No foreground application found.");
    return "";
}

bool checkHostInstalledAppIsCherry(const string &appID) {
    LOGINFO();

    string response = getCurlResponse("0:9005/as/apps");

    JsonObject joResponse(response);
    if (joResponse.HasLabel("apps")) {
        JsonArray apps = joResponse["apps"].Array();
        int len = apps.Length();
        if (len > 0) {
            auto j_itr = apps.Elements();
            j_itr.Reset();
            for (int i = 0; i < len && j_itr.Next(); i++) {
                JsonObject app = j_itr.Current().Object();
                if (app.HasLabel("appId") && app["appId"].Value() == appID) {
                    if (app.HasLabel("cherryApp") && app["cherryApp"].Boolean() == true) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool launchAMP(const std::string &appID, const std::string &appContentData) {
    string params;
    JsonObject joParams;
    joParams.Set("origin", "APP");
    joParams.Set("SKU", appID);
    joParams.Set("originAppId", appID);

    if (!appContentData.empty()) {
        JsonObject joArgs;
        joArgs.Set("offerCode", appContentData);
        joParams.Set("args", joArgs);
    }

    joParams.ToString(params);

    string response = getCurlResponse("0:9001/as/apps/subscription/action/launch", params);
    // Note this request currently returns empty response.
    // JsonObject joResponse(response);
    // if (joResponse.HasLabel("result")) {
    //     JsonObject result = joResponse["result"].Object();
    //     if (result.HasLabel("success") && result["success"].Boolean() == true) {
    //         return true;
    //     }
    // }
    return true;
}

// Helper function to perform a JSONRPC request and return response as string.
std::string callPersistentStoreRPC(const std::string &method, const std::string &appID, const std::string &key,
                                   const std::string &scope = "", const std::string &value = "") {
    JsonObject joRequest;
    joRequest.Set("jsonrpc", "2.0");
    joRequest.Set("id", rand() % 9998);
    joRequest.Set("method", method);

    JsonObject joParams;
    joParams.Set("namespace", appID);
    joParams.Set("key", key);
    joParams.Set("scope", scope);
    if (!value.empty()) {
        joParams.Set("value", value);
    }
    joRequest.Set("params", joParams);

#ifdef USE_THUNDER_COMMUNICATION
    JsonObject joResult;
    int32_t status = Utils::getThunderControllerClient()->Invoke(20000, method.c_str(), joRequest, joResult);
    string result;
    joResult.ToString(result);
    return result;
#else
    string params;
    joRequest.ToString(params);
return getCurlResponse("http://127.0.0.1:9998/jsonrpc", params);
#endif
}

std::string getAppToken(const std::string &appID, const std::string &key, const std::string &scope) {
    std::string response = callPersistentStoreRPC("org.rdk.PersistentStore.getValue", appID, key, scope, "");
    JsonObject joResponse(response);
    if (joResponse.HasLabel("result")) {
        JsonObject result(joResponse["result"].Object());
        if (result.HasLabel("success") && result["success"].Boolean() && result.HasLabel("value")) {
            return result["value"].Value();
        }
    }

    return std::string();
}

bool setAppToken(const std::string &appID, const std::string &key, const std::string &value, const std::string &scope) {
    std::string response = callPersistentStoreRPC("org.rdk.PersistentStore.setValue", appID, key, scope, value);
    JsonObject joResponse(response);
    return joResponse.HasLabel("result") && joResponse["result"].Object().HasLabel("success") &&
           joResponse["result"].Object()["success"].Boolean();
}

bool deleteAppToken(const std::string &appID, const std::string &key, const std::string &scope) {
    std::string response = callPersistentStoreRPC("org.rdk.PersistentStore.deleteKey", appID, key, scope, "");
    JsonObject joResponse(response);
    return joResponse.HasLabel("result") && joResponse["result"].Object().HasLabel("success") &&
           joResponse["result"].Object()["success"].Boolean();
}

} // namespace Plugin
} // namespace WPEFramework
