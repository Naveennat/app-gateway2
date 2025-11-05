#pragma once

/**
 * MetricsHandlerDelegate
 *
 * Header-only delegate that sends Badger metrics through the Analytics bridge (org.rdk.Analytics),
 * aligning payload shapes to the FbMetrics schema and dispatching via Exchange::IAnalytics::SendEvent.
 *
 * Notes:
 * - This delegate supports optional appId propagation to Analytics if available from caller.
 * - Arguments (name/value pairs) can be flattened into a "parameters" object on the payload.
 * - Analytics plugin enriches timestamps; we pass default values aligning to FbMetrics defaults.
 */

#include <string>
#include <vector>
#include <memory>

#include <core/JSON.h>
#include <plugins/plugins.h>
#include <interfaces/IAnalytics.h>

#include "UtilsCallsign.h"
#include "UtilsLogging.h"
#include "StringUtils.h"

namespace MetricsHandler {

    // PUBLIC_INTERFACE
    struct ParamKV {
        /** Name-value pair for additional parameters. */
        std::string name;
        Core::JSON::Variant value;

        ParamKV() = default;
        ParamKV(const std::string& n, const Core::JSON::Variant& v) : name(n), value(v) {}
    };

    enum class HandlerEventType {
        UserAction,
        AppAction,
        PageView,
        Error,
        UserError
    };

    // Helper constants aligned with FbMetrics defaults
    static constexpr const char* EVENT_SOURCE_DEFAULT         = "ripple";
    static constexpr const char* EVENT_SOURCE_VERSION_DEFAULT = "3.5.0";
    static constexpr const char* EVENT_VERSION_DEFAULT        = "3";

    // Convert Core::JSON::Variant to JsonValue for embedding in JsonObject
    static inline JsonValue VariantToJsonValue(const Core::JSON::Variant& v) {
        JsonValue out;
        if (v.IsString()) {
            out = v.String();
        } else if (v.IsNumber()) {
            out = v.Number();
        } else if (v.IsBoolean()) {
            out = v.Boolean();
        } else if (v.IsNull()) {
            out = JsonValue(); // null
        } else if (v.IsObject()) {
            Core::JSON::String tmp;
            v.Object().ToString(tmp);
            JsonObject obj;
            obj.FromString(tmp.Value());
            out = obj;
        } else if (v.IsArray()) {
            Core::JSON::String tmp;
            v.Array().ToString(tmp);
            JsonArray arr;
            arr.FromString(tmp.Value());
            out = arr;
        } else {
            // Fallback: serialize to string
            out = v.Serialize();
        }
        return out;
    }

    // PUBLIC_INTERFACE
    class MetricsHandlerDelegate {
    public:
        /**
         * Construct a MetricsHandlerDelegate with a Thunder shell for COM-RPC queries.
         */
        explicit MetricsHandlerDelegate(PluginHost::IShell* shell)
            : _shell(shell) {
        }

        ~MetricsHandlerDelegate() = default;

        // PUBLIC_INTERFACE
        /**
         * Generic metrics handler: maps to an in-app action event (category "app") with type from segment.
         * Args are flattened into "parameters".
         */
        Core::hresult MetricsHandler(const Core::OptionalType<std::string>& segment,
                                     const std::vector<ParamKV>& args) const
        {
            return MetricsHandler(segment, args, "");
        }

        // PUBLIC_INTERFACE
        /**
         * Generic metrics handler with appId forwarding.
         */
        Core::hresult MetricsHandler(const Core::OptionalType<std::string>& segment,
                                     const std::vector<ParamKV>& args,
                                     const std::string& appId) const
        {
            std::string eventName = "inapp_other_action";

            JsonObject eventPayload;
            // Map "segment" to a generic app action type (preserve as-is).
            if (segment.IsSet() && !segment.Value().empty()) {
                eventPayload["category"] = "app";
                eventPayload["type"] = segment.Value();
            } else {
                eventPayload["category"] = "app";
                eventPayload["type"] = "metrics"; // generic
            }

            JsonObject parameters;
            ArgsToParametersObject(args, parameters);
            if (parameters.Length() > 0) {
                eventPayload["parameters"] = parameters;
            }

            return SendViaAnalytics(eventName, eventPayload, appId);
        }

        // PUBLIC_INTERFACE
        /**
         * Launch Completed metrics handler.
         * Normalized as an app action event: "inapp_other_action" { category: "app", type: "launch_completed" }
         */
        Core::hresult LaunchCompleted(const std::vector<ParamKV>& args) const {
            return LaunchCompleted(args, "");
        }

        // PUBLIC_INTERFACE
        /**
         * Launch Completed with appId.
         */
        Core::hresult LaunchCompleted(const std::vector<ParamKV>& args, const std::string& appId) const {
            std::string eventName = "inapp_other_action";

            JsonObject eventPayload;
            eventPayload["category"] = "app";
            eventPayload["type"] = "launch_completed";

            JsonObject parameters;
            ArgsToParametersObject(args, parameters);
            if (parameters.Length() > 0) {
                eventPayload["parameters"] = parameters;
            }

            return SendViaAnalytics(eventName, eventPayload, appId);
        }

        // PUBLIC_INTERFACE
        /**
         * User Action handler.
         * Normalized to FbMetrics Action: "inapp_other_action" with category "user" and type=<action>.
         */
        Core::hresult UserAction(const std::string& action, const std::vector<ParamKV>& args) const {
            return UserAction(action, args, "");
        }

        // PUBLIC_INTERFACE
        /**
         * User Action with appId.
         */
        Core::hresult UserAction(const std::string& action, const std::vector<ParamKV>& args, const std::string& appId) const {
            if (!ValidateActionType(action)) {
                LOGERR("MetricsHandlerDelegate::UserAction: action length out of range [1..256]");
                return Core::ERROR_BAD_REQUEST;
            }

            std::string eventName = "inapp_other_action";
            JsonObject eventPayload;
            eventPayload["category"] = "user";
            eventPayload["type"] = action;

            JsonObject parameters;
            ArgsToParametersObject(args, parameters);
            if (parameters.Length() > 0) {
                eventPayload["parameters"] = parameters;
            }

            return SendViaAnalytics(eventName, eventPayload, appId);
        }

        // PUBLIC_INTERFACE
        /**
         * App Action handler.
         * Normalized to FbMetrics Action: "inapp_other_action" with category "app" and type=<action>.
         */
        Core::hresult AppAction(const std::string& action, const std::vector<ParamKV>& args) const {
            return AppAction(action, args, "");
        }

        // PUBLIC_INTERFACE
        /**
         * App Action with appId.
         */
        Core::hresult AppAction(const std::string& action, const std::vector<ParamKV>& args, const std::string& appId) const {
            if (!ValidateActionType(action)) {
                LOGERR("MetricsHandlerDelegate::AppAction: action length out of range [1..256]");
                return Core::ERROR_BAD_REQUEST;
            }

            std::string eventName = "inapp_other_action";
            JsonObject eventPayload;
            eventPayload["category"] = "app";
            eventPayload["type"] = action;

            JsonObject parameters;
            ArgsToParametersObject(args, parameters);
            if (parameters.Length() > 0) {
                eventPayload["parameters"] = parameters;
            }

            return SendViaAnalytics(eventName, eventPayload, appId);
        }

        // PUBLIC_INTERFACE
        /**
         * Page View handler: "inapp_page_view" with src_page_id=<page>.
         */
        Core::hresult PageView(const std::string& page, const std::vector<ParamKV>& args) const {
            return PageView(page, args, "");
        }

        // PUBLIC_INTERFACE
        /**
         * Page View handler with appId.
         */
        Core::hresult PageView(const std::string& page, const std::vector<ParamKV>& args, const std::string& appId) const {
            if (page.empty()) {
                LOGERR("MetricsHandlerDelegate::PageView: page cannot be empty");
                return Core::ERROR_BAD_REQUEST;
            }

            std::string eventName = "inapp_page_view";
            JsonObject eventPayload;
            eventPayload["src_page_id"] = page;

            // Optional parameters
            JsonObject parameters;
            ArgsToParametersObject(args, parameters);
            if (parameters.Length() > 0) {
                eventPayload["parameters"] = parameters;
            }

            return SendViaAnalytics(eventName, eventPayload, appId);
        }

        // PUBLIC_INTERFACE
        /**
         * User Error handler: "app_error" with type="UserError", code, description (err_msg), visible and parameters.
         */
        Core::hresult UserError(const std::string& message, bool visible, const std::string& code,
                                const std::vector<ParamKV>& args) const
        {
            return UserError(message, visible, code, args, "");
        }

        // PUBLIC_INTERFACE
        /**
         * User Error handler with appId.
         */
        Core::hresult UserError(const std::string& message, bool visible, const std::string& code,
                                const std::vector<ParamKV>& args, const std::string& appId) const
        {
            if (message.empty()) {
                LOGERR("MetricsHandlerDelegate::UserError: err_msg cannot be empty");
                return Core::ERROR_BAD_REQUEST;
            }

            std::string eventName = "app_error";
            JsonObject eventPayload;
            eventPayload["type"] = "UserError";
            eventPayload["code"] = code;
            eventPayload["description"] = message;
            eventPayload["visible"] = visible;

            JsonObject parameters;
            ArgsToParametersObject(args, parameters);
            if (parameters.Length() > 0) {
                eventPayload["parameters"] = parameters;
            }

            // Mirror FbMetrics::Error which sets third_party_error true
            eventPayload["third_party_error"] = true;

            return SendViaAnalytics(eventName, eventPayload, appId);
        }

        // PUBLIC_INTERFACE
        /**
         * App Error handler: "app_error" with type="Error", code, description (err_msg), visible and parameters.
         */
        Core::hresult Error(const std::string& message, bool visible, const std::string& code,
                            const std::vector<ParamKV>& args) const
        {
            return Error(message, visible, code, args, "");
        }

        // PUBLIC_INTERFACE
        /**
         * App Error handler with appId.
         */
        Core::hresult Error(const std::string& message, bool visible, const std::string& code,
                            const std::vector<ParamKV>& args, const std::string& appId) const
        {
            if (message.empty()) {
                LOGERR("MetricsHandlerDelegate::Error: err_msg cannot be empty");
                return Core::ERROR_BAD_REQUEST;
            }

            std::string eventName = "app_error";
            JsonObject eventPayload;
            eventPayload["type"] = "Error";
            eventPayload["code"] = code;
            eventPayload["description"] = message;
            eventPayload["visible"] = visible;

            JsonObject parameters;
            ArgsToParametersObject(args, parameters);
            if (parameters.Length() > 0) {
                eventPayload["parameters"] = parameters;
            }

            // Mirror FbMetrics::Error which sets third_party_error true
            eventPayload["third_party_error"] = true;

            return SendViaAnalytics(eventName, eventPayload, appId);
        }

        // PUBLIC_INTERFACE
        /**
         * HandleBadgerMetrics
         * Encapsulates logic for badger.metricsHandler:
         *  - Accepts JSON::VariantContainer params
         *  - Flattens optional 'evt' object into args (name/value list)
         *  - Routes by 'segment' or 'eventType' to typed methods
         *  - For unknown/missing keys, sends generic MetricsHandler
         *
         * Returns Core::ERROR_* status from underlying send, but callers may ignore it for fire-and-forget.
         */
        Core::hresult HandleBadgerMetrics(const std::string& appId,
                                          const Core::JSON::VariantContainer& params) const
        {
            // Flatten evt -> args
            std::vector<ParamKV> args = FlattenEvt(params);

            const Core::JSON::Variant segVar = params["segment"];
            if (segVar.IsSet() && !segVar.IsNull() && segVar.Content() == Core::JSON::Variant::type::STRING) {
                const std::string segment = segVar.String();
                if (segment == "LAUNCH_COMPLETED") {
                    return LaunchCompleted(args, appId);
                }
                Core::OptionalType<std::string> seg(segment);
                return MetricsHandler(seg, args, appId);
            }

            const Core::JSON::Variant evtTypeVar = params["eventType"];
            if (evtTypeVar.IsSet() && !evtTypeVar.IsNull() && evtTypeVar.Content() == Core::JSON::Variant::type::STRING) {
                const std::string eventType = evtTypeVar.String();
                const std::string eventTypeLower = StringUtils::toLower(eventType);

                if (eventTypeLower == "useraction") {
                    const std::string action = ReadString(params, "action");
                    return UserAction(action, args, appId);
                } else if (eventTypeLower == "appaction") {
                    const std::string action = ReadString(params, "action");
                    return AppAction(action, args, appId);
                } else if (eventTypeLower == "pageview") {
                    const std::string page = ReadString(params, "page");
                    return PageView(page, args, appId);
                } else if (eventTypeLower == "usererror" || eventTypeLower == "error") {
                    const std::string errMsg  = ReadString(params, "errMsg");
                    const std::string errCode = ReadString(params, "errCode");
                    const bool errVisible     = ReadBool(params, "errVisible", false);
                    if (eventTypeLower == "usererror") {
                        return UserError(errMsg, errVisible, errCode, args, appId);
                    } else {
                        return Error(errMsg, errVisible, errCode, args, appId);
                    }
                } else {
                    // Unknown eventType: route as generic app action with type=eventType
                    Core::OptionalType<std::string> seg(eventType);
                    return MetricsHandler(seg, args, appId);
                }
            }

            // Neither 'segment' nor 'eventType' provided, still send generic metrics to avoid drop
            Core::OptionalType<std::string> empty;
            return MetricsHandler(empty, args, appId);
        }

    private:
        static inline bool ValidateActionType(const std::string& s) {
            return s.size() >= 1 && s.size() <= 256;
        }

        static inline void ArgsToParametersObject(const std::vector<ParamKV>& src, JsonObject& out) {
            out.Clear();
            for (const auto& kv : src) {
                out[kv.name] = VariantToJsonValue(kv.value);
            }
        }

        static std::string ReadString(const Core::JSON::VariantContainer& obj, const char* key) {
            const Core::JSON::Variant v = obj[key];
            if (v.IsSet() && !v.IsNull() && v.Content() == Core::JSON::Variant::type::STRING) {
                return v.String();
            }
            return std::string();
        }

        static bool ReadBool(const Core::JSON::VariantContainer& obj, const char* key, bool defVal) {
            const Core::JSON::Variant v = obj[key];
            if (v.IsSet() && !v.IsNull() && v.Content() == Core::JSON::Variant::type::BOOLEAN) {
                return v.Boolean();
            }
            return defVal;
        }

        static std::vector<ParamKV> FlattenEvt(const Core::JSON::VariantContainer& params) {
            std::vector<ParamKV> out;
            const Core::JSON::Variant evtVar = params["evt"];
            if (evtVar.IsSet() && !evtVar.IsNull() && evtVar.Content() == Core::JSON::Variant::type::OBJECT) {
                Core::JSON::VariantContainer evtObj = evtVar.Object();
                auto it = evtObj.Variants();
                while (it.Next()) {
                    ParamKV kv;
                    kv.name = it.Label();
                    kv.value = it.Current();
                    out.emplace_back(std::move(kv));
                }
            }
            return out;
        }

        /**
         * Send normalized event via Analytics bridge (COM-RPC) using IAnalytics::SendEvent.
         * Uses:
         *  - eventVersion="3"
         *  - eventSource="ripple"
         *  - eventSourceVersion="3.5.0"
         *  - cetList: empty (Analytics may enrich)
         *  - timestamps: 0 (Analytics handles)
         *  - appId: caller-provided (can be empty)
         */
        Core::hresult SendViaAnalytics(const std::string& eventName, const JsonObject& eventPayload, const std::string& appId) const {
            if (_shell == nullptr) {
                LOGERR("MetricsHandlerDelegate: Shell is not initialized");
                return Core::ERROR_UNAVAILABLE;
            }

            // Create empty iterator (same as FbMetrics)
            WPEFramework::Exchange::IAnalytics::IStringIterator* cetList =
                (WPEFramework::Core::Service<WPEFramework::RPC::StringIterator>::Create<WPEFramework::RPC::IStringIterator>(std::list<string>()));
            if (cetList == nullptr) {
                LOGERR("MetricsHandlerDelegate: Failed to create empty IStringIterator.");
                return Core::ERROR_UNAVAILABLE;
            }

            auto analyticsInterface = _shell->QueryInterfaceByCallsign<WPEFramework::Exchange::IAnalytics>(ANALYTICS_PLUGIN_CALLSIGN);
            if (analyticsInterface == nullptr) {
                LOGERR("MetricsHandlerDelegate: Analytics interface (%s) not found.", ANALYTICS_PLUGIN_CALLSIGN);
                return Core::ERROR_UNAVAILABLE;
            }

            std::string payloadStr;
            eventPayload.ToString(payloadStr);

            Core::hresult result = analyticsInterface->SendEvent(
                eventName, EVENT_VERSION_DEFAULT, EVENT_SOURCE_DEFAULT, EVENT_SOURCE_VERSION_DEFAULT,
                cetList, 0 /*epoch*/, 0 /*uptime*/, appId, payloadStr);

            analyticsInterface->Release();

            if (result != Core::ERROR_NONE) {
                LOGERR("MetricsHandlerDelegate: SendEvent failed for '%s' with rc=%d", eventName.c_str(), result);
            }
            return result;
        }

        // Backward-compatible helper for existing internal calls.
        Core::hresult SendViaAnalytics(const std::string& eventName, const JsonObject& eventPayload) const {
            return SendViaAnalytics(eventName, eventPayload, "");
        }

    private:
        PluginHost::IShell* _shell;
    };

} // namespace MetricsHandler
