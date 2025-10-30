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
#include "ProviderDelegate.h"
#include "StringUtils.h"
#include "ObjectUtils.h"
#include "UtilsLogging.h"
#include "UtilsFirebolt.h"
#include "UtilsUUID.h"
#include <interfaces/IAppNotifications.h>
namespace WPEFramework {
namespace Plugin {

    Core::hresult ProviderDelegate::HandleAppGatewayRequest(const Exchange::GatewayContext& context ,
                                          const string& method ,
                                          const string& payload ,
                                          const string& origin ,
                                          string& result ) 
    {
        LOGINFO("HandleAppGatewayRequest: method=%s, payload=%s, appId=%s",
                method.c_str(), payload.c_str(), context.appId.c_str());
        JsonObject payloadObj;

        if(payloadObj.FromString(payload)) {
            JAdditionalProviderContext additionalContext;
            if (ExtractAdditionalContext(payload, additionalContext)) {
                JsonObject paramsObj = payloadObj.Get("params").Object();
                if (paramsObj.IsSet()) {
                    ProviderMethodType providerType = GetType(additionalContext.providerType.Value());
                    std::string capability = StringUtils::toLower(additionalContext.capability.Value());
                    switch (providerType) {
                        case ProviderMethodType::REGISTER: {
                            bool provide = false;
                            if (!ObjectUtils::HasBooleanEntry(paramsObj, "listen", provide)) {
                                ErrorUtils::CustomBadRequest("Missing listen parameter", result);
                                return Core::ERROR_BAD_REQUEST;
                            }
                            auto providerResult = Core::ERROR_NONE == RegisterProvider(
                                {
                                    context.requestId,
                                    context.connectionId,
                                    context.appId,
                                    origin
                                },
                                capability,
                                provide,
                                result
                            );
                            if (providerResult == Core::ERROR_NONE) {
                                JListenResponse response;
                                response.event = method;
                                response.listening = provide;
                                response.ToString(result);
                            } else if(result.empty()) {
                                LOGERR("Failed to register provider for capability: %s", capability.c_str());
                                ErrorUtils::CustomBadRequest("Failed to register provider", result);
                            }
                            return providerResult;
                        }
                        case ProviderMethodType::INVOKE: {
                            return InvokeProvider(
                                {
                                    context.requestId,
                                    context.connectionId,
                                    context.appId,
                                    origin
                                },
                                capability,
                                paramsObj,
                                result
                            );
                        }
                        case ProviderMethodType::RESULT: {
                            return HandleProviderResponse(
                                paramsObj,
                                result
                            );
                        }
                        case ProviderMethodType::ERROR: {
                            return HandleProviderError(
                                paramsObj,
                                result
                            );

                        }
                        case ProviderMethodType::NOTIFY: {
                            return HandleProviderNotification(
                                capability,
                                paramsObj,
                                result
                            );
                        }
                        default:
                            ErrorUtils::CustomBadRequest("Unsupported Provider type", result);
                            return Core::ERROR_GENERAL;
                    }
                } else {
                    LOGERR("Failed to parse payload JSON: %s", payload.c_str());
                    ErrorUtils::CustomBadRequest("Bad Parms failure", result);
                    return Core::ERROR_GENERAL;
                }
            } else {
                LOGERR("Failed to parse payload JSON: %s", payload.c_str());
                ErrorUtils::CustomBadRequest("Missing additional context failure", result);
                return Core::ERROR_GENERAL;
            }
        } else {
            LOGERR("Failed to parse payload JSON: %s", payload.c_str());
            ErrorUtils::CustomBadRequest("Parse failure", result);
            return Core::ERROR_GENERAL;
        }

    }

    Core::hresult ProviderDelegate::RegisterProvider(const ProviderContext &context ,
                                                    const string& capability ,
                                                   const bool& provide,
                                                   string& result
                                                   ) {
        string lower_c = StringUtils::toLower(capability);
        if (provide) {
            mProviderRegistry.Add(lower_c, context);
            // Add additional entry for cases where same app provides capability with a suffix
            // for eg create is a capability and multiple apps can create the same capability
            // we will have 2 entries <create,context> and <create.appId, context>
            if (!context.appId.empty()) {
                string lower_app_id = StringUtils::toLower(context.appId);
                string combinedKey = lower_c + "." + lower_app_id;
                mProviderRegistry.Add(combinedKey, context);
            }
        } else {
            // Even if same capability is provided by multiple apps, we will remove both entries
            // Given there is an established precedence that this particular capability is a composite
            // key. For any provider deregistration we need to clean up the entry with just the
            // capability. From this point onwards the Consumer MUST make the request with appId
            // which is registered as the only other available provider
            mProviderRegistry.Remove(lower_c);
            if (!context.appId.empty()) {
                string lower_app_id = StringUtils::toLower(context.appId);
                string combinedKey = lower_c + "." + lower_app_id;
                mProviderRegistry.Remove(combinedKey);
            }
        }
        return Core::ERROR_NONE;
    }

    Core::hresult ProviderDelegate::InvokeProvider(const ProviderContext &context ,
                                                 const string& capability ,
                                                 const JsonObject& params,
                                                 string& result
                                                 ) {
        string lower_c = StringUtils::toLower(capability);
        
        ProviderContext providerContext;
        // Check if params has an entry called appId string
        // if so get the appId
        if (params.HasLabel("appId") && params["appId"].IsSet() && params["appId"].Content() == JsonValue::type::STRING) {
            string appId = params["appId"].String();
            if (!appId.empty()) {
                string lower_app_id = StringUtils::toLower(appId);
                string combinedKey = lower_c + "." + lower_app_id;
                if (mProviderRegistry.Get(combinedKey, providerContext)) {
                    BrokerProvider(context, providerContext, params);
                    return Core::ERROR_NONE;
                }
            }
        }

        if (mProviderRegistry.Get(lower_c, providerContext)) {
            BrokerProvider(context, providerContext, params);
            return Core::ERROR_NONE;
        } else {
            LOGERR("No Provider available");
        }
        
        ErrorUtils::CustomInternal("Invoke failure", result);
        return Core::ERROR_GENERAL;
    }

    Core::hresult ProviderDelegate::HandleProviderResponse(const JsonObject& params , string& result) {
        string correlationId;
        string resultStr;
        if (ExtractCorrelationIdAndKey(params, "result", correlationId, resultStr)) {
            ProviderContext requestContext;
            if (mTransactionRegistry.Get(correlationId, requestContext)) {
                mTransactionRegistry.Remove(correlationId);

                // Forward response to gateway or launch delegate based on origin
                if (_respondCallback) {
                    _respondCallback(requestContext, resultStr);
                } else {
                    LOGERR("Respond callback not set");
                }
                return Core::ERROR_NONE;
            } else {
                LOGERR("No matching transaction for correlationId %s", correlationId.c_str());
            }
        } else {
            LOGERR("No Correlation ID");
        }
        ErrorUtils::CustomInternal("Response handle failure", result);
        return Core::ERROR_GENERAL;
    }

    Core::hresult ProviderDelegate::HandleProviderError(const JsonObject& params , string& result) {
        string correlationId;
        string resultStr;
        if (ExtractCorrelationIdAndKey(params, "error", correlationId, resultStr)) {
            ProviderContext requestContext;
            if (mTransactionRegistry.Get(correlationId, requestContext)) {
                mTransactionRegistry.Remove(correlationId);

                // Forward response to gateway or launch delegate based on origin
                if (_respondCallback) {
                    _respondCallback(requestContext, resultStr);
                } else {
                    LOGERR("Respond callback not set");
                }
                return Core::ERROR_NONE;
            } else {
                LOGERR("No matching transaction for correlationId %s", correlationId.c_str());
            }
        } else {
            LOGERR("No Correlation ID");
        }
        ErrorUtils::CustomInternal("Error handle failure", result);
        return Core::ERROR_GENERAL;
    }

    Core::hresult ProviderDelegate::HandleProviderNotification(const string& capability, const JsonObject& params , string& result) {
        string payload;
        params.ToString(payload);
        if (_emitCallback) {
            _emitCallback(capability, payload, "");
        }
        return Core::ERROR_NONE;
    }

    void ProviderDelegate::Cleanup(const uint32_t connectionId , const string &origin ) {
        LOGINFO("Cleanup: connectionId=%u, origin=%s",
                connectionId, origin.c_str());
        mProviderRegistry.Cleanup(connectionId, origin);
        mTransactionRegistry.Cleanup(connectionId, origin);
    }

    bool ProviderDelegate::ExtractAdditionalContext(const string &payload, JAdditionalProviderContext& additionalContext) {
        JsonObject payloadObj;
        if(!payloadObj.FromString(payload)) {
            LOGERR("Failed to parse payload JSON: %s", payload.c_str());
            return false;
        }
        std::string contextStr = payloadObj.Get("additionalContext").Value();
        if (contextStr.empty()) {
            LOGERR("additionalContext not found in payload");
            return false;
        }

        if (!additionalContext.FromString(contextStr)) {
            LOGERR("Failed to convert additionalContext to JSON object");
            return false;
        }
        std::string error;
        if (!additionalContext.Validate(error)) {
            LOGERR("additionalContext validation failed: %s", error.c_str());
            return false;
        }
        return true;
    }

    ProviderMethodType ProviderDelegate::GetType(const std::string& typeStr) {
        std::string lowerTypeAlias = StringUtils::toLower(typeStr);
        auto it = ProviderMethodTypeMap.find(lowerTypeAlias);
        if (it != ProviderMethodTypeMap.end()) {
            return it->second;
        }
        return ProviderMethodType::NONE; 
    }

    void ProviderDelegate::BrokerProvider(const ProviderContext &context, const ProviderContext &providerContext, const JsonObject paramsObject) {
        // Check if paramsObject has an entry called appId string
        // if so get the appId
        string correlationId = UtilsUUID::GenerateUUID();
        mTransactionRegistry.Add(correlationId, context);
        JsonObject object;
        object["correlationId"] = correlationId;
        object["params"] = paramsObject;
        string dispatchParams;
        object.ToString(dispatchParams);
        LOGINFO("Invoking provider %s with correlationId %s", dispatchParams.c_str(), correlationId.c_str());
        // Dispatch to provider
        if (_respondCallback) {
            _respondCallback(providerContext, dispatchParams);
        } else {
            LOGERR("Respond callback not set");
        }

    }

    bool ProviderDelegate::ExtractCorrelationIdAndKey(const JsonObject &object, const string &key, string& correlationId, string& result ) {
        if (object.IsSet()) {
        JsonValue cid = object["correlationId"];
        if (cid.IsSet() && !cid.IsNull()) {
            correlationId = cid.String();
            JsonValue resultObject = object[key.c_str()];
            if (resultObject.IsSet() && !resultObject.String().empty()) {
                    result = resultObject.String();
                    return true;
                } else {
                    result = "null";
                }
                return true;
            } else {
                LOGERR("correlationId missing in responseObject");
            }
        } else {
            LOGERR("responseObject parse failure");
        }
        return false;
    }
}

}
