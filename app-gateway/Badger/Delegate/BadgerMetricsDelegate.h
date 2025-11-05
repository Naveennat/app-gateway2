#pragma once

/**
 * BadgerMetricsDelegate
 *
 * Option A: Route Badger metrics via the Analytics bridge (org.rdk.Analytics),
 * using the same COM-RPC path and payload schema normalization as FbMetrics::SendAnalyticsEvent.
 *
 * - This implementation normalizes event payloads to match the shapes built in FbMetrics.
 * - It then dispatches events using Exchange::IAnalytics::SendEvent via COM-RPC,
 *   using the ANALYTICS_PLUGIN_CALLSIGN ("org.rdk.Analytics").
 * - We do not change FbMetrics behavior; we only adapt Badger to use the same bridge and schema.
 *
 * Note:
 * - Badger delegate methods here do not receive an app context, so we cannot populate
 *   app_session_id, app_user_session_id, durable_app_id, or app_version as FbMetrics does.
 *   Those are optional in the schema and will be omitted by this delegate.
 * - We flatten args (vector<ParamKV>) into a "parameters" object, analogous to FbMetrics.
 * - The Analytics plugin will enrich timestamps; we pass 0 for epoch and uptime to mirror FbMetrics default here.
 */

#include <string>
#include <vector>
#include <memory>

#include <core/JSON.h>
#include <plugins/plugins.h>
#include <interfaces/IAnalytics.h>

#include "UtilsCallsign.h"
#include "UtilsLogging.h"

namespace BadgerMetrics {

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
    class BadgerMetricsDelegate {
    public:
        /**
         * Construct a BadgerMetricsDelegate with a Thunder shell for COM-RPC queries.
         */
        explicit BadgerMetricsDelegate(PluginHost::IShell* shell)
            : _shell(shell) {
        }

        ~BadgerMetricsDelegate() = default;

        // PUBLIC_INTERFACE
        /**
         * Generic metrics handler: maps to an in-app action event (category "app") with type from segment.
         * Args are flattened into "parameters".
         */
        Core::hresult MetricsHandler(const Core::OptionalType<std::string>& segment,
                                     const std::vector<ParamKV>& args) const
        {
            std::string eventName = "inapp_other_action";

            JsonObject eventPayload;
            // Map "segment" to a generic app action type (lowercase transformation optional).
            if (segment.IsSet() && !segment.Value().empty()) {
                eventPayload["category"] = "app";
                eventPayload["type"] = segment.Value(); // Preserve as-is; consumer can map
            } else {
                eventPayload["category"] = "app";
                eventPayload["type"] = "metrics"; // generic
            }

            JsonObject parameters;
            ArgsToParametersObject(args, parameters);
            if (parameters.Length() > 0) {
                eventPayload["parameters"] = parameters;
            }

            return SendViaAnalytics(eventName, eventPayload);
        }

        // PUBLIC_INTERFACE
        /**
         * Launch Completed metrics handler.
         * Normalized as an app action event: "inapp_other_action" { category: "app", type: "launch_completed" }
         */
        Core::hresult LaunchCompleted(const std::vector<ParamKV>& args) const {
            std::string eventName = "inapp_other_action";

            JsonObject eventPayload;
            eventPayload["category"] = "app";
            eventPayload["type"] = "launch_completed";

            JsonObject parameters;
            ArgsToParametersObject(args, parameters);
            if (parameters.Length() > 0) {
                eventPayload["parameters"] = parameters;
            }

            return SendViaAnalytics(eventName, eventPayload);
        }

        // PUBLIC_INTERFACE
        /**
         * User Action handler.
         * Normalized to FbMetrics Action: event "inapp_other_action" with category "user" and type=<action>.
         */
        Core::hresult UserAction(const std::string& action, const std::vector<ParamKV>& args) const {
            if (!ValidateActionType(action)) {
                LOGERR("UserAction: action length out of range [1..256]");
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

            return SendViaAnalytics(eventName, eventPayload);
        }

        // PUBLIC_INTERFACE
        /**
         * App Action handler.
         * Normalized to FbMetrics Action: event "inapp_other_action" with category "app" and type=<action>.
         */
        Core::hresult AppAction(const std::string& action, const std::vector<ParamKV>& args) const {
            if (!ValidateActionType(action)) {
                LOGERR("AppAction: action length out of range [1..256]");
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

            return SendViaAnalytics(eventName, eventPayload);
        }

        // PUBLIC_INTERFACE
        /**
         * Page View handler.
         * Normalized to FbMetrics Page: event "inapp_page_view" with src_page_id=<page>.
         */
        Core::hresult PageView(const std::string& page, const std::vector<ParamKV>& args) const {
            if (page.empty()) {
                LOGERR("PageView: page cannot be empty");
                return Core::ERROR_BAD_REQUEST;
            }

            std::string eventName = "inapp_page_view";
            JsonObject eventPayload;
            eventPayload["src_page_id"] = page;

            // Enrich optional parameters if any
            JsonObject parameters;
            ArgsToParametersObject(args, parameters);
            if (parameters.Length() > 0) {
                // FbMetrics does not send parameters for page view, but carrying them does not break schema.
                eventPayload["parameters"] = parameters;
            }

            return SendViaAnalytics(eventName, eventPayload);
        }

        // PUBLIC_INTERFACE
        /**
         * User Error handler.
         * Normalized to FbMetrics Error: event "app_error" with type="UserError", code, description (err_msg), visible and parameters.
         */
        Core::hresult UserError(const std::string& message, bool visible, const std::string& code,
                                const std::vector<ParamKV>& args) const
        {
            if (message.empty()) {
                LOGERR("UserError: err_msg cannot be empty");
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

            return SendViaAnalytics(eventName, eventPayload);
        }

        // PUBLIC_INTERFACE
        /**
         * App Error handler.
         * Normalized to FbMetrics Error: event "app_error" with type="Error", code, description (err_msg), visible and parameters.
         */
        Core::hresult Error(const std::string& message, bool visible, const std::string& code,
                            const std::vector<ParamKV>& args) const
        {
            if (message.empty()) {
                LOGERR("Error: err_msg cannot be empty");
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

            return SendViaAnalytics(eventName, eventPayload);
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

        /**
         * Send normalized event via Analytics bridge (COM-RPC) using IAnalytics::SendEvent.
         * Uses:
         *  - eventVersion="3"
         *  - eventSource="ripple"
         *  - eventSourceVersion="3.5.0"
         *  - cetList: empty (Analytics may enrich)
         *  - timestamps: 0 (Analytics handles)
         *  - appId: empty (no context available in Badger delegate)
         */
        Core::hresult SendViaAnalytics(const std::string& eventName, const JsonObject& eventPayload) const {
            if (_shell == nullptr) {
                LOGERR("BadgerMetricsDelegate: Shell is not initialized");
                return Core::ERROR_UNAVAILABLE;
            }

            // Create empty iterator (same as FbMetrics)
            WPEFramework::Exchange::IAnalytics::IStringIterator* cetList =
                (WPEFramework::Core::Service<WPEFramework::RPC::StringIterator>::Create<WPEFramework::RPC::IStringIterator>(std::list<string>()));
            if (cetList == nullptr) {
                LOGERR("BadgerMetricsDelegate: Failed to create empty IStringIterator.");
                return Core::ERROR_UNAVAILABLE;
            }

            auto analyticsInterface = _shell->QueryInterfaceByCallsign<WPEFramework::Exchange::IAnalytics>(ANALYTICS_PLUGIN_CALLSIGN);
            if (analyticsInterface == nullptr) {
                LOGERR("BadgerMetricsDelegate: Analytics interface (%s) not found.", ANALYTICS_PLUGIN_CALLSIGN);
                return Core::ERROR_UNAVAILABLE;
            }

            std::string payloadStr;
            eventPayload.ToString(payloadStr);

            // No appId context is available here; pass empty string
            const std::string appId = "";

            Core::hresult result = analyticsInterface->SendEvent(
                eventName, EVENT_VERSION_DEFAULT, EVENT_SOURCE_DEFAULT, EVENT_SOURCE_VERSION_DEFAULT,
                cetList, 0 /*epoch*/, 0 /*uptime*/, appId, payloadStr);

            analyticsInterface->Release();

            if (result != Core::ERROR_NONE) {
                LOGERR("BadgerMetricsDelegate: SendEvent failed for '%s' with rc=%d", eventName.c_str(), result);
            }
            return result;
        }

    private:
        PluginHost::IShell* _shell;
    };

} // namespace BadgerMetrics
