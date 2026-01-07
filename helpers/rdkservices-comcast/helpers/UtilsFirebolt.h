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

#ifndef __UTILSFIREBOLT_H__
#define __UTILSFIREBOLT_H__

#include <mutex>
#include <map>
#include <unordered_map>

using namespace WPEFramework;

/**
 * Firebolt JSON-RPC error codes.
 *
 * IMPORTANT:
 * Do NOT use generic macro names like ERROR_NOT_SUPPORTED because they collide with
 * Thunder's Core::ERROR_NOT_SUPPORTED enum constant (and similar), breaking builds via
 * macro expansion (e.g., Core::ERROR_NOT_SUPPORTED => Core::(-50100)).
 */
#define FIREBOLT_ERROR_NOT_SUPPORTED (-50100)
#define FIREBOLT_ERROR_NOT_AVAILABLE (-50200)
#define FIREBOLT_ERROR_NOT_PERMITTED (-40300)
#define FIREBOLT_ERROR_GRANT_DENIED (-40400)
#define FIREBOLT_ERROR_UNGRANTED (-40401)
#define FIREBOLT_ERROR_APP_NOT_IN_ACTIVE_STATE (-40402)
#define FIREBOLT_ERROR_GRANT_PROVIDER_MISSING (-40403)

class JListenRequest: public Core::JSON::Container {
    public:
        Core::JSON::Boolean listen;
    
    JListenRequest() : Core::JSON::Container()
    {
        Add(_T("listen"), &listen);
    }

    bool Get() const {
        return listen.Value();
    }
};

class JListenResponse: public Core::JSON::Container {
    public:
        Core::JSON::String event;
        Core::JSON::Boolean listening;
    
    JListenResponse() : Core::JSON::Container()
    {
        Add(_T("event"), &event);
        Add(_T("listening"), &listening);
    }
    
};

struct ProviderInfo{
    uint32_t channelId{0};
    uint32_t requestId{0};

    static ProviderInfo create(uint32_t chId, uint32_t reqId) {
        ProviderInfo info;
        info.channelId = chId;
        info.requestId = reqId;
        return info;
    }
};

class ProviderRegistry{
    public:
    void Add(const std::string& key, uint32_t chId, uint32_t reqId) {
        std::lock_guard<std::mutex> lock(mProviderMutex);
        mProviderMap[key] = ProviderInfo::create(chId, reqId);
    }

    void Add(const std::string& key, ProviderInfo provider) {
        std::lock_guard<std::mutex> lock(mProviderMutex);
        mProviderMap[key] = provider;
    }

    void Remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(mProviderMutex);
        mProviderMap.erase(key);
    }

    void CleanupByConnectionId(const uint32_t connectionId) {
        std::lock_guard<std::mutex> lock(mProviderMutex);
        for (auto it = mProviderMap.begin(); it != mProviderMap.end(); ) {
            if (it->second.channelId == connectionId) {
                it = mProviderMap.erase(it);
            } else {
                ++it;
            }
        }
    }

    ProviderInfo Get(const std::string& key) {
        ProviderInfo info;
        std::lock_guard<std::mutex> lock(mProviderMutex);
        auto it = mProviderMap.find(key);
        if (it != mProviderMap.end()) {
            info = it->second;
        }
        return info;
    }

      private:            
        std::unordered_map<std::string, ProviderInfo> mProviderMap;
        std::mutex mProviderMutex;
};

enum FireboltError {
    NOT_SUPPORTED,
    NOT_AVAILABLE,
    NOT_PERMITTED,
    GRANT_DENIED,
    UNGRANTED,
    APP_NOT_IN_ACTIVE_STATE,
    GRANT_PROVIDER_MISSING
};


class ErrorUtils {
    public:
    static std::string GetErrorMessageForFrameworkErrors(const uint32_t& errorCode, const std::string& message) {
        std::string errorMessage;
        Core::JSONRPC::Message::Info info;
        info.SetError(errorCode);
        info.Text = message;
        info.ToString(errorMessage);
        return errorMessage;
    }

    static std::string GetFireboltError(const FireboltError& error) {
        std::string errorMessage;
        Core::JSONRPC::Message::Info info;
        switch (error) {
            case FireboltError::NOT_SUPPORTED:
                info.Code = FIREBOLT_ERROR_NOT_SUPPORTED;
                info.Text = "NotSupported";
                break;
            case FireboltError::NOT_AVAILABLE:
                info.Code = FIREBOLT_ERROR_NOT_AVAILABLE;
                info.Text = "NotAvailable";
                break;
            case FireboltError::NOT_PERMITTED:
                info.Code = FIREBOLT_ERROR_NOT_PERMITTED;
                info.Text = "NotPermitted";
                break;
            case FireboltError::GRANT_DENIED:
                info.Code = FIREBOLT_ERROR_GRANT_DENIED;
                info.Text = "GrantDenied";
                break;
            case FireboltError::UNGRANTED:
                info.Code = FIREBOLT_ERROR_UNGRANTED;
                info.Text = "Ungranted";
                break;
            case FireboltError::APP_NOT_IN_ACTIVE_STATE:
                info.Code = FIREBOLT_ERROR_APP_NOT_IN_ACTIVE_STATE;
                info.Text = "AppNotInActiveState";
                break;
            case FireboltError::GRANT_PROVIDER_MISSING:
                info.Code = FIREBOLT_ERROR_GRANT_PROVIDER_MISSING;
                info.Text = "GrantProviderMissing";
                break;
            default:
                errorMessage = "UnknownError";
                break;
        }
        info.ToString(errorMessage);
        return errorMessage;
    }

    static void NotSupported(string& resolution) {
        // L0 tests expect Thunder Core error semantics for common gateway failures.
        // Map NotSupported -> Core::ERROR_NOT_SUPPORTED (commonly 24).
        resolution = ErrorUtils::GetErrorMessageForFrameworkErrors(Core::ERROR_NOT_SUPPORTED, "NotSupported");
    }

    static void NotAvailable(string& resolution) {
        // Map NotAvailable -> Core::ERROR_UNAVAILABLE (commonly 2).
        resolution = ErrorUtils::GetErrorMessageForFrameworkErrors(Core::ERROR_UNAVAILABLE, "NotAvailable");
    }

    static void NotPermitted(string& resolution) {
        // Map NotPermitted -> Thunder "privileged request" error.
        // NOTE: Thunder spells this constant as ERROR_PRIVILIGED_REQUEST (legacy spelling).
        resolution = ErrorUtils::GetErrorMessageForFrameworkErrors(Core::ERROR_PRIVILIGED_REQUEST, "NotPermitted");
    }

    static void GrantDenied(string& resolution) {
        resolution = ErrorUtils::GetFireboltError(FireboltError::GRANT_DENIED);
    }

    static void Ungranted(string& resolution) {
        resolution = ErrorUtils::GetFireboltError(FireboltError::UNGRANTED);
    }

    static void AppNotInActiveState(string& resolution) {
        resolution = ErrorUtils::GetFireboltError(FireboltError::APP_NOT_IN_ACTIVE_STATE);
    }

    static void GrantProviderMissing(string& resolution) {
        resolution = ErrorUtils::GetFireboltError(FireboltError::GRANT_PROVIDER_MISSING);
    }

    static void CustomInitialize(const string& message, string& resolution) {
        resolution = ErrorUtils::GetErrorMessageForFrameworkErrors(Core::ERROR_GENERAL, message);
    }

    static void CustomInternal(const string& message, string& resolution) {
        // "Internal error" should not be reported as a client-side bad request.
        resolution = ErrorUtils::GetErrorMessageForFrameworkErrors(Core::ERROR_GENERAL, message);
    }

    static void CustomBadRequest(const string& message, string& resolution) {
        // Use the canonical Thunder bad request code.
        resolution = ErrorUtils::GetErrorMessageForFrameworkErrors(Core::ERROR_BAD_REQUEST, message);
    }

    static void CustomBadMethod(const string& message, string& resolution) {
        // L0 expects method-not-found to map to ERROR_UNKNOWN_METHOD (53).
        resolution = ErrorUtils::GetErrorMessageForFrameworkErrors(Core::ERROR_UNKNOWN_METHOD, message);
    }
    
};

#endif