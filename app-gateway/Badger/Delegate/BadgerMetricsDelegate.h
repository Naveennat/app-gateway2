#pragma once

/**
 * BadgerMetricsDelegate
 *
 * Implements handlers for sending Money Badger metrics via the existing JSON-RPC
 * direct link mechanism, following the payload structure referenced by:
 *   - Supporting_Files/ripple-eos/src/rpc/eos_metrics_rpc.rs
 *   - Supporting_Files/ripple-eos/src/model/metrics.rs
 *   - Supporting_Files/Badger_helper_files/metrics.json (JSON schema for metrics events)
 *
 * Notes:
 * - We attempt to validate payloads against metrics.json if present and valid JSON.
 *   The provided file currently contains HTML (SSO/blocked content). If parsing fails,
 *   we log and fallback to structural validations aligned with the Rust models to ensure
 *   correctness without blocking compilation.
 * - We ignore timing values (e.g., Ready.ttmu_ms) as requested.
 * - We emit/enqueue metrics using the same JSONRPC direct link utility (UtilsJsonrpcDirectLink.h)
 *   used by other delegates in this project.
 */

#include <string>
#include <vector>
#include <utility>
#include <memory>

#include "DelegateUtils.h"
#include <core/JSON.h>
#include <plugins/plugins.h>
#include "UtilsLogging.h"

#ifndef EOS_METRICS_CALLSIGN
// Callsign for the EOS/Ripple metrics plugin. Adjust if your runtime uses a different callsign.
#define EOS_METRICS_CALLSIGN "RippleEos"
#endif

namespace BadgerMetrics {

    // Lightweight typed parameter for Badger args: { name: string, value: any }
    struct ParamKV {
        std::string name;
        Core::JSON::Variant value; // Holds a typed JSON value

        ParamKV() = default;
        ParamKV(const std::string& n, const Core::JSON::Variant& v) : name(n), value(v) {}
    };

    // Utility to convert a collection of ParamKV to the expected args array format:
    // args: [{ "name": <string>, "value": <flatMapValue> }, ...]
    inline void BuildArgsArray(const std::vector<ParamKV>& src, JsonArray& out) {
        out.Clear();
        for (const auto& kv : src) {
            JsonObject entry;
            entry["name"] = kv.name;
            // Serialize Core::JSON::Variant to object value
            if (kv.value.IsString()) {
                entry["value"] = kv.value.String();
            } else if (kv.value.IsNumber()) {
                // JsonValue can hold a double; preserve numeric semantics
                entry["value"] = kv.value.Number();
            } else if (kv.value.IsBoolean()) {
                entry["value"] = kv.value.Boolean();
            } else if (kv.value.IsNull()) {
                // Explicit null; still allowable
                entry["value"] = JsonValue();
            } else if (kv.value.IsObject()) {
                // VariantContainer serialization
                Core::JSON::String tmp;
                kv.value.Object().ToString(tmp);
                JsonObject vo;
                vo.FromString(tmp.Value());
                entry["value"] = vo;
            } else if (kv.value.IsArray()) {
                // Variant array -> JsonArray
                Core::JSON::String tmp;
                kv.value.Array().ToString(tmp);
                JsonArray va;
                va.FromString(tmp.Value());
                entry["value"] = va;
            } else {
                // Fallback: stringify
                entry["value"] = kv.value.Serialize();
            }
            out.Add(std::move(entry));
        }
    }

    // Basic runtime validation results
    struct ValidationResult {
        bool ok {true};
        std::string message {};

        static ValidationResult Success() { return {true, ""}; }
        static ValidationResult Error(const std::string& msg) { return {false, msg}; }
    };

    enum class HandlerEventType {
        UserAction,
        AppAction,
        PageView,
        Error,
        UserError
    };

    // Internal builder for the single EOS metrics RPC endpoint.
    // Target method per eos_metrics_rpc.rs: "eos.metricsHandler"
    class BadgerMetricsDelegate {
    public:
        explicit BadgerMetricsDelegate(PluginHost::IShell* shell)
            : _shell(shell),
              _eosLink(DelegateUtils::AcquireLink(shell, EOS_METRICS_CALLSIGN)),
              _schemaPath("/home/kavia/workspace/code-generation/app-gateway2/Supporting_Files/Badger_helper_files/metrics.json") {
        }

        ~BadgerMetricsDelegate() = default;

        // PUBLIC_INTERFACE
        /**
         * Send a generic metrics handler payload through the EOS metrics RPC.
         * This method is the underlying call used by the more specialized handlers.
         * It constructs the JSON object according to BadgerMetricsHandlerParams shape.
         *
         * @param segment optional segment string (e.g., "LAUNCH_COMPLETED")
         * @param evtType optional typed event designation (UserAction, AppAction, PageView, Error, UserError)
         * @param action optional action string (for action handlers)
         * @param page optional page string (for page view handler)
         * @param errMsg optional error message (for error handlers)
         * @param errVisible optional error visibility flag (for error handlers)
         * @param errCode optional error code (string form; will be forwarded as-is)
         * @param args optional vector of ParamKV (will be mapped to args: [{name,value}])
         * @return Core::hresult
         */
        Core::hresult SendMetricsHandler(
            const Core::OptionalType<std::string>& segment,
            const Core::OptionalType<HandlerEventType>& evtType,
            const Core::OptionalType<std::string>& action,
            const Core::OptionalType<std::string>& page,
            const Core::OptionalType<std::string>& errMsg,
            const Core::OptionalType<bool>& errVisible,
            const Core::OptionalType<std::string>& errCode,
            const std::vector<ParamKV>& args) const
        {
            if (!_eosLink) {
                LOGERR("BadgerMetricsDelegate: EOS link unavailable");
                return Core::ERROR_UNAVAILABLE;
            }

            JsonObject params;
            if (segment.IsSet()) {
                params["segment"] = segment.Value();
            }
            if (evtType.IsSet()) {
                params["eventType"] = ToEventTypeString(evtType.Value());
            }
            if (action.IsSet()) {
                // Rust expects "action" inside params for action mappings
                params["action"] = action.Value();
            }
            if (page.IsSet()) {
                params["page"] = page.Value();
            }
            if (errMsg.IsSet()) {
                params["err_msg"] = errMsg.Value();
            }
            if (errVisible.IsSet()) {
                params["err_visible"] = errVisible.Value();
            }
            if (errCode.IsSet()) {
                params["err_code"] = errCode.Value();
            }

            if (!args.empty()) {
                JsonArray argsArr;
                BuildArgsArray(args, argsArr);
                params["args"] = argsArr;
            }

            // Execute validations: first try to validate against the schema file,
            // otherwise fallback to specific structural checks.
            const ValidationResult vr = ValidateParams("eos.metricsHandler", params);
            if (!vr.ok) {
                LOGERR("BadgerMetricsDelegate: validation failed: %s", vr.message.c_str());
                return Core::ERROR_BAD_REQUEST;
            }

            JsonObject response;
            // The EOS RPC method referenced in eos_metrics_rpc.rs
            // Designator called will be "<EOS_METRICS_CALLSIGN>.1.eos.metricsHandler"
            uint32_t rc = _eosLink->Invoke<JsonObject, JsonObject>(_T("eos.metricsHandler"), params, response);
            if (rc != Core::ERROR_NONE) {
                LOGERR("BadgerMetricsDelegate: eos.metricsHandler failed, rc=%u", rc);
                return rc; // propagate error
            }
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        /**
         * badger.metricsHandler.metrics Handler
         * Generic convenience entry for metric segments.
         * Will forward any args as the args array (Param objects).
         */
        Core::hresult MetricsHandler(
            const Core::OptionalType<std::string>& segment,
            const std::vector<ParamKV>& args) const
        {
            return SendMetricsHandler(segment, Core::OptionalType<HandlerEventType>(), Core::OptionalType<std::string>(),
                                      Core::OptionalType<std::string>(), Core::OptionalType<std::string>(),
                                      Core::OptionalType<bool>(), Core::OptionalType<std::string>(), args);
        }

        // PUBLIC_INTERFACE
        /**
         * badger.metricsHandler.Launch Completed metrics handler
         * Sends a segment of "LAUNCH_COMPLETED" and triggers ready semantics in EOS (if supported).
         * Args are optional for additional values, but timing values are ignored by design.
         */
        Core::hresult LaunchCompleted(const std::vector<ParamKV>& args) const {
            return SendMetricsHandler(Core::OptionalType<std::string>(std::string("LAUNCH_COMPLETED")),
                                      Core::OptionalType<HandlerEventType>(), Core::OptionalType<std::string>(),
                                      Core::OptionalType<std::string>(), Core::OptionalType<std::string>(),
                                      Core::OptionalType<bool>(), Core::OptionalType<std::string>(), args);
        }

        // PUBLIC_INTERFACE
        /**
         * badger.metricsHandler.user Action Metrics Handler
         * Sends a user action. The 'action' string is validated for length (1..256) per validate_metrics_action_type()
         */
        Core::hresult UserAction(const std::string& action, const std::vector<ParamKV>& args) const {
            if (!ValidateActionType(action)) {
                LOGERR("UserAction: action length out of range [1..256]");
                return Core::ERROR_BAD_REQUEST;
            }
            return SendMetricsHandler(Core::OptionalType<std::string>(), Core::OptionalType<HandlerEventType>(HandlerEventType::UserAction),
                                      Core::OptionalType<std::string>(action), Core::OptionalType<std::string>(),
                                      Core::OptionalType<std::string>(), Core::OptionalType<bool>(), Core::OptionalType<std::string>(), args);
        }

        // PUBLIC_INTERFACE
        /**
         * badger.metricsHandler.app Action Metrics Handler
         * Sends an app action. The 'action' string is validated for length (1..256).
         */
        Core::hresult AppAction(const std::string& action, const std::vector<ParamKV>& args) const {
            if (!ValidateActionType(action)) {
                LOGERR("AppAction: action length out of range [1..256]");
                return Core::ERROR_BAD_REQUEST;
            }
            return SendMetricsHandler(Core::OptionalType<std::string>(), Core::OptionalType<HandlerEventType>(HandlerEventType::AppAction),
                                      Core::OptionalType<std::string>(action), Core::OptionalType<std::string>(),
                                      Core::OptionalType<std::string>(), Core::OptionalType<bool>(), Core::OptionalType<std::string>(), args);
        }

        // PUBLIC_INTERFACE
        /**
         * badger.metricsHandler.page View Metrics Handler
         * Sends a page view metric with a non-empty page string.
         */
        Core::hresult PageView(const std::string& page, const std::vector<ParamKV>& args) const {
            if (page.empty()) {
                LOGERR("PageView: page cannot be empty");
                return Core::ERROR_BAD_REQUEST;
            }
            return SendMetricsHandler(Core::OptionalType<std::string>(), Core::OptionalType<HandlerEventType>(HandlerEventType::PageView),
                                      Core::OptionalType<std::string>(), Core::OptionalType<std::string>(page),
                                      Core::OptionalType<std::string>(), Core::OptionalType<bool>(), Core::OptionalType<std::string>(), args);
        }

        // PUBLIC_INTERFACE
        /**
         * badger.metricsHandler.user Error Metrics Handler
         * Sends a user-level error metric with message, visibility and code.
         * Code is provided as a string and forwarded unchanged to the EOS handler.
         */
        Core::hresult UserError(const std::string& message, bool visible, const std::string& code,
                                const std::vector<ParamKV>& args) const {
            if (message.empty()) {
                LOGERR("UserError: err_msg cannot be empty");
                return Core::ERROR_BAD_REQUEST;
            }
            return SendMetricsHandler(Core::OptionalType<std::string>(), Core::OptionalType<HandlerEventType>(HandlerEventType::UserError),
                                      Core::OptionalType<std::string>(), Core::OptionalType<std::string>(),
                                      Core::OptionalType<std::string>(message),
                                      Core::OptionalType<bool>(visible),
                                      Core::OptionalType<std::string>(code),
                                      args);
        }

        // PUBLIC_INTERFACE
        /**
         * badger.metricsHandler.Error Metrics Handler
         * Sends an app-level error metric with message, visibility and code.
         */
        Core::hresult Error(const std::string& message, bool visible, const std::string& code,
                            const std::vector<ParamKV>& args) const {
            if (message.empty()) {
                LOGERR("Error: err_msg cannot be empty");
                return Core::ERROR_BAD_REQUEST;
            }
            return SendMetricsHandler(Core::OptionalType<std::string>(), Core::OptionalType<HandlerEventType>(HandlerEventType::Error),
                                      Core::OptionalType<std::string>(), Core::OptionalType<std::string>(),
                                      Core::OptionalType<std::string>(message),
                                      Core::OptionalType<bool>(visible),
                                      Core::OptionalType<std::string>(code),
                                      args);
        }

    private:
        static inline string ToEventTypeString(HandlerEventType t) {
            switch (t) {
                case HandlerEventType::UserAction: return "UserAction";
                case HandlerEventType::AppAction:  return "AppAction";
                case HandlerEventType::PageView:   return "PageView";
                case HandlerEventType::Error:      return "Error";
                case HandlerEventType::UserError:  return "UserError";
            }
            return "Unknown";
        }

        // Mirroring validate_metrics_action_type in eos_metrics_rpc.rs
        static inline bool ValidateActionType(const std::string& s) {
            return s.size() >= 1 && s.size() <= 256;
        }

        // Try to validate using schema file if it is valid JSON; otherwise fallback to structural checks.
        ValidationResult ValidateParams(const std::string& methodName, const JsonObject& params) const {
            // Attempt to read and parse the schema file (if present)
            string schemaText;
            if (Core::File(_schemaPath).Exists()) {
                Core::File f(_schemaPath);
                if (f.Open(true) == true) {
                    uint64_t size = f.Size();
                    if (size > 0) {
                        string buffer;
                        buffer.resize(size);
                        f.Read(reinterpret_cast<uint8_t*>(&buffer[0]), size);
                        f.Close();

                        // Quick sanity: detect accidental HTML (SSO page)
                        if (buffer.find("<!DOCTYPE html>") != string::npos || buffer.find("<html") != string::npos) {
                            LOGWARN("Badger metrics schema appears to be HTML (blocked). Falling back to structural validation.");
                        } else {
                            // Basic JSON check
                            JsonObject schemaObj;
                            if (schemaObj.FromString(buffer) == true) {
                                // We have a valid JSON object loaded; real JSON-schema validation is not implemented in this project.
                                // For now, we log presence and continue with structural checks.
                                LOGDBG("Badger metrics schema JSON loaded; proceeding with structural checks.");
                            } else {
                                LOGWARN("Badger metrics schema not parseable as JSON. Falling back to structural validation.");
                            }
                        }
                    }
                }
            } else {
                LOGWARN("Badger metrics schema file not found at %s", _schemaPath.c_str());
            }

            // Structural validation aligned with eos_metrics_rpc.rs
            // Note: We only validate the fields we set and care about.
            if (methodName == "eos.metricsHandler") {
                if (params.HasLabel("eventType")) {
                    const auto typeStr = params["eventType"].String();
                    if (typeStr != "UserAction" && typeStr != "AppAction" && typeStr != "PageView" &&
                        typeStr != "Error" && typeStr != "UserError") {
                        return ValidationResult::Error("Invalid eventType");
                    }
                }
                if (params.HasLabel("action")) {
                    const auto actionStr = params["action"].String();
                    if (!ValidateActionType(actionStr)) {
                        return ValidationResult::Error("action out of range [1..256]");
                    }
                }
                if (params.HasLabel("page")) {
                    const auto pageStr = params["page"].String();
                    if (pageStr.empty()) {
                        return ValidationResult::Error("page cannot be empty");
                    }
                }
                if (params.HasLabel("err_msg")) {
                    const auto errStr = params["err_msg"].String();
                    if (errStr.empty()) {
                        return ValidationResult::Error("err_msg cannot be empty");
                    }
                }
                if (params.HasLabel("err_visible")) {
                    // accept boolean
                    if (!params["err_visible"].IsBoolean()) {
                        return ValidationResult::Error("err_visible must be boolean");
                    }
                }
                if (params.HasLabel("err_code")) {
                    // accept string; EOS will interpret downstream
                    if (!params["err_code"].IsString()) {
                        return ValidationResult::Error("err_code must be string");
                    }
                }
                if (params.HasLabel("args")) {
                    if (!params["args"].ContentType().empty()) {
                        // Accept array only
                        const auto& value = params["args"];
                        if (!value.IsArray()) {
                            return ValidationResult::Error("args must be an array");
                        }
                    }
                }
            }

            return ValidationResult::Success();
        }

    private:
        PluginHost::IShell* _shell;
        std::shared_ptr<WPEFramework::Utils::JSONRPCDirectLink> _eosLink;
        const std::string _schemaPath;
    };

} // namespace BadgerMetrics
