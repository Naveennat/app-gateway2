/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
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
*/

#include <interfaces/IAppGateway.h>
#include <mutex>
#include <map>
namespace WPEFramework {
namespace Plugin {
    using Context = Exchange::GatewayContext;
    struct ProviderContext
    {
        uint32_t requestId;       // @brief Unique identifier for the request.
        uint32_t connectionId; // @brief connectionId for the execution/session context.
        string appId;        // @brief Application identifier (Firebolt appId).
        string origin;      // @brief Origin of the request (e.g., org.rdk.AppGateway or org.rdk.LaunchDelegate).
    };

    class JAdditionalProviderContext: public Core::JSON::Container {
        public:
        Core::JSON::String providerType; // Provider type
        Core::JSON::String capability; // Application catalog info
        
        JAdditionalProviderContext() : Core::JSON::Container()
        {
            Add(_T("providerType"), &providerType);
            Add(_T("capability"), &capability);
            
        }
        
        bool Validate(string& error) const {
            if (!providerType.IsSet()) {
                error = "Provider type is not set";
                return false;
            }
            if (!capability.IsSet()) {
                error = "Capability is not set";
                return false;
            }
            return true;
        }

    };

     enum ProviderMethodType: uint8_t
    {
        NONE = 0,
        REGISTER,
        INVOKE,
        RESULT,
        NOTIFY,
        ERROR
    };

    static const std::unordered_map<std::string, ProviderMethodType> ProviderMethodTypeMap = {
            {"", ProviderMethodType::NONE},
            {"register", ProviderMethodType::REGISTER},
            {"invoke", ProviderMethodType::INVOKE},
            {"response", ProviderMethodType::RESULT},
            {"notify", ProviderMethodType::NOTIFY},
            {"error", ProviderMethodType::ERROR}
    };

    template <typename KeyType>
        class RegistryMap {
        public:
            RegistryMap(){}
            ~RegistryMap() {
                std::lock_guard<std::mutex> lock(mContextMutex);
                mContextMap.clear();
            }
            void Add(const KeyType& key, const ProviderContext& value) {
                std::lock_guard<std::mutex> lock(mContextMutex);
                mContextMap[key] = value;
            }

            void Remove(const KeyType& key) {
                std::lock_guard<std::mutex> lock(mContextMutex);
                mContextMap.erase(key);
            }

            bool Get(const KeyType& key, ProviderContext& value) {
                std::lock_guard<std::mutex> lock(mContextMutex);
                auto it = mContextMap.find(key);
                if (it != mContextMap.end()) {
                    value = it->second;
                    return true;
                }
                return false;
            }

            bool Cleanup(const uint32_t& connectionId, const string& origin) {
                std::lock_guard<std::mutex> lock(mContextMutex);
                bool removed = false;
                for (auto it = mContextMap.begin(); it != mContextMap.end(); ) {
                    if (it->second.connectionId == connectionId && it->second.origin == origin) {
                        it = mContextMap.erase(it);
                        removed = true;
                    } else {
                        ++it;
                    }
                }
                return removed;
            }

            std::unordered_map<KeyType, ProviderContext> mContextMap;
            std::mutex mContextMutex;
        };

    class ProviderDelegate {

        public:
        
        using RespondCallback = std::function<void(const ProviderContext& context, const std::string& payload)>;
        using EmitCallback = std::function<void(const string& event, const std::string& payload, const std::string& appId)>;

        void SetRespondCallback(const RespondCallback& callback) { _respondCallback = callback; }

        void SetEmitCallback(const EmitCallback& callback) { _emitCallback = callback; }

        Core::hresult RegisterProvider(const ProviderContext &context ,
                                                const string& capability ,
                                                const bool& provide,
                                                string& result
                                                );
        
        Core::hresult InvokeProvider(const ProviderContext &context ,
                                                const string& capability ,
                                                const JsonObject& params,
                                                string& result
                                                );

        Core::hresult HandleProviderResponse(const JsonObject& params , string& result);

        Core::hresult HandleProviderError(const JsonObject& params , string& result);

        Core::hresult HandleProviderNotification( const string& capability, const JsonObject& params , string& result);

        void Cleanup(const uint32_t connectionId , const string &origin );


        Core::hresult HandleAppGatewayRequest(const Exchange::GatewayContext& context ,
                                          const string& method ,
                                          const string& payload ,
                                          const string& origin ,
                                          string& result
                                        );
                                          
        private:

        bool ExtractAdditionalContext(const string &payload, JAdditionalProviderContext& additionalContext);

        ProviderMethodType GetType(const std::string& typeStr);

        RespondCallback _respondCallback;
        EmitCallback _emitCallback;
        // Instantiate RegistryMap for KeyType of string and ValueType of Context
        RegistryMap<std::string> mProviderRegistry;

        // Transaction map
        RegistryMap<std::string> mTransactionRegistry;
        void BrokerProvider(const ProviderContext &context, const ProviderContext &providerContext, const JsonObject paramsObject);
        bool ExtractCorrelationIdAndKey(const JsonObject &object, const string &key, string& correlationId, string& result );
    };
}
}