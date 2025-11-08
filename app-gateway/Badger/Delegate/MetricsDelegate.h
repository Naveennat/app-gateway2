#pragma once

/**
 * MetricsDelegate
 *
 * Delegate that sends Badger metrics through the Analytics bridge (org.rdk.Analytics),
 * aligning payload shapes to the FbMetrics schema and dispatching via Exchange::IAnalytics::SendEvent.
 *
 * Structure mirrors SystemDelegate pattern:
 *  - Header-only implementation with a PluginHost::IShell* dependency
 *  - Public methods for specific metric actions
 *  - Private helpers for payload composition and Analytics invocation
 *
 * Any orchestration or routing logic (e.g., handling variations of the input payload for
 * badger.metricsHandler) has been moved into Badger.cpp as a helper method
 * (Badger::HandleMetricsProcessing).
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

namespace WPEFramework {
    class MetricsDelegate {
    public:
        // PUBLIC_INTERFACE
        struct ParamKV {
            /** Name-value pair for additional parameters. */
            std::string name;
            Core::JSON::Variant value;

            ParamKV() = default;
            ParamKV(const std::string& n, const Core::JSON::Variant& v) : name(n), value(v) {}
        };

    public:
        // PUBLIC_INTERFACE
        explicit MetricsDelegate(PluginHost::IShell* shell)
            : _shell(shell) {
        }

        // PUBLIC_INTERFACE
        ~MetricsDelegate() = default;

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
                LOGERR("MetricsDelegate::UserAction: action length out of range [1..256]");
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
                LOGERR("MetricsDelegate::AppAction: action length out of range [1..256]");
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
                LOGERR("MetricsDelegate::PageView: page cannot be empty");
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
                LOGERR("MetricsDelegate::UserError: err_msg cannot be empty");
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
                LOGERR("MetricsDelegate::Error: err_msg cannot be empty");
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
                LOGERR("MetricsDelegate: Shell is not initialized");
                return Core::ERROR_UNAVAILABLE;
            }

            // Create empty iterator (same as FbMetrics)
            WPEFramework::Exchange::IAnalytics::IStringIterator* cetList =
                (WPEFramework::Core::Service<WPEFramework::RPC::StringIterator>::Create<WPEFramework::RPC::IStringIterator>(std::list<string>()));
            if (cetList == nullptr) {
                LOGERR("MetricsDelegate: Failed to create empty IStringIterator.");
                return Core::ERROR_UNAVAILABLE;
            }

            auto analyticsInterface = _shell->QueryInterfaceByCallsign<WPEFramework::Exchange::IAnalytics>(ANALYTICS_PLUGIN_CALLSIGN);
            if (analyticsInterface == nullptr) {
                LOGERR("MetricsDelegate: Analytics interface (%s) not found.", ANALYTICS_PLUGIN_CALLSIGN);
                return Core::ERROR_UNAVAILABLE;
            }

            std::string payloadStr;
            eventPayload.ToString(payloadStr);

            Core::hresult result = analyticsInterface->SendEvent(
                eventName, "3", "ripple", "3.5.0",
                cetList, 0 /*epoch*/, 0 /*uptime*/, appId, payloadStr);

            analyticsInterface->Release();

            if (result != Core::ERROR_NONE) {
                LOGERR("MetricsDelegate: SendEvent failed for '%s' with rc=%d", eventName.c_str(), result);
            }
            return result;
        }

        // Backward-compatible helper for internal calls.
        Core::hresult SendViaAnalytics(const std::string& eventName, const JsonObject& eventPayload) const {
            return SendViaAnalytics(eventName, eventPayload, "");
        }

    private:
        PluginHost::IShell* _shell;
        mutable Core::CriticalSection mAdminLock;
    };
}  // namespace WPEFramework
