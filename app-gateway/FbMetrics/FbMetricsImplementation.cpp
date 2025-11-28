/*
 * Copyright 2023 Comcast Cable Communications Management, LLC
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
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "FbMetricsImplementation.h"
#include "UtilsLogging.h"
#include "UtilsCallsign.h"

#include <interfaces/IAnalytics.h>
#include <interfaces/IAppGateway.h>

#define FBMETRICS_EVENT_SOURCE_DFL "ripple"
#define FBMETRICS_EVENT_SOURCE_VERSION_DFL "3.5.0"
#define FBMETRICS_EVENT_VERSION_DFL "3"

namespace WPEFramework
{
    namespace Plugin
    {

        SERVICE_REGISTRATION(FbMetricsImplementation, 1, 0);

        FbMetricsImplementation::FbMetricsImplementation() : mShell(nullptr), mAppVersionCache(), mAppUserSessionIdRegistry()
        {
        }

        FbMetricsImplementation::~FbMetricsImplementation()
        {
            // Cleanup resources if needed
            if (mShell != nullptr)
            {
                mShell->Release();
                mShell = nullptr;
            }
        }

        Core::hresult FbMetricsImplementation::Action(const Exchange::IFbMetrics::Context &context,
                                                      const string &category,
                                                      const string &type,
                                                      const string &parameters)
        {
            LOGINFO("[appId=%s] Action: category=%s, type=%s, parameters=%s",
                    context.appId.c_str(),
                    category.c_str(), type.c_str(), parameters.c_str());

            string eventName = "inapp_other_action";

            //"eventPayload": {
            // "category": "user",
            // "type": "SignIn Prompt Displayed",     // renamed internally as action_type
            // "parameters": { "key": "value", ... }, // flattened optional map
            // "app_session_id": "<filled_by_update_app_context>",
            // "app_user_session_id": "<maybe_null>",
            // "durable_app_id": "<ctx.app_id>",
            // "app_version": "<if set via metrics.appInfo>"
            // }
            JsonObject eventPayload;
            eventPayload["category"] = category;
            eventPayload["type"] = type;

            if (!parameters.empty())
            {
                Core::OptionalType<Core::JSON::Error> error;
                JsonObject json;
                if (!json.FromString(parameters, error))
                {
                    LOGERR("parameters is not a JSON object: %s , %s", parameters.c_str(),
                           (error.IsSet() ? error.Value().Message().c_str() : "Unknown"));

                    return Core::ERROR_BAD_REQUEST;
                }

                eventPayload["parameters"] = json;
            }

            UpdateWithAppContext(context, eventPayload);

            string eventPayloadStr;
            eventPayload.ToString(eventPayloadStr);

            return SendAnalyticsEvent(eventName, eventPayloadStr, context.appId);
        }

        Core::hresult FbMetricsImplementation::AppInfo(const Exchange::IFbMetrics::Context &context,
                                                       const string &build)
        {
            LOGINFO("appId=%s] AppInfo: build=%s",
                    context.appId.c_str(), build.c_str());

            // Cache the app version
            mAppVersionCache.Add(context.appId, build);

            return Core::ERROR_NONE;
        }

        Core::hresult FbMetricsImplementation::Error(const Exchange::IFbMetrics::Context &context,
                                                     const string &type,
                                                     const string &code,
                                                     const string &description,
                                                     const bool visible,
                                                     const string &parameters)
        {
            LOGINFO("appId=%s] Error: type=%s, code=%s, visible=%d, description=%s, parameters=%s",
                    context.appId.c_str(),
                    type.c_str(), code.c_str(), visible, description.c_str(), parameters.c_str());

            string eventName = "app_error";

            //  "eventPayload": {
            //   "type": "<type>",
            //   "code": "<string code>",
            //   "description": "<short description>",
            //   "visible": true,
            //   "parameters": { "...": ... },          // optional; omitted if None
            //   "app_session_id": "<from update_app_context>",        // optional
            //   "app_user_session_id": "<maybe null>",                // optional
            //   "durable_app_id": "<ctx.app_id>",
            //   "app_version": "<from metrics.appInfo>",              // optional
            //   "third_party_error": true
            // }

            JsonObject eventPayload;
            eventPayload["type"] = type;
            eventPayload["code"] = code;
            eventPayload["description"] = description;
            eventPayload["visible"] = visible;
            if (!parameters.empty())
            {
                Core::OptionalType<Core::JSON::Error> error;
                JsonObject json;
                if (!json.FromString(parameters, error))
                {
                    LOGERR("parameters is not a JSON object: %s , %s", parameters.c_str(),
                           (error.IsSet() ? error.Value().Message().c_str() : "Unknown"));

                    return Core::ERROR_BAD_REQUEST;
                }

                eventPayload["parameters"] = json;
            }

            UpdateWithAppContext(context, eventPayload);
            eventPayload["third_party_error"] = true;

            string eventPayloadStr;
            eventPayload.ToString(eventPayloadStr);

            return SendAnalyticsEvent(eventName, eventPayloadStr, context.appId);
        }

        Core::hresult FbMetricsImplementation::MediaEnded(const Exchange::IFbMetrics::Context &context,
                                                          const string &entityId)
        {
            LOGINFO("appId=%s] MediaEnded: entityId=%s",
                    context.appId.c_str(), entityId.c_str());

            string eventName = "inapp_media";

            // "eventPayload": {
            //     "media_event_name": "mediaEnded",
            //     "src_entity_id": "<entityId>",
            //     "app_session_id": "<from update_app_context>",
            //     "app_user_session_id": "<empty string if None>",
            //     "durable_app_id": "<ctx.app_id>",
            //     "app_version": "<from metrics.appInfo (optional)>"
            //   }

            JsonObject eventPayload;
            eventPayload["media_event_name"] = "mediaEnded";
            eventPayload["src_entity_id"] = entityId;
            UpdateWithAppContext(context, eventPayload);

            string eventPayloadStr;
            eventPayload.ToString(eventPayloadStr);

            return SendAnalyticsEvent(eventName, eventPayloadStr, context.appId);
        }

        Core::hresult FbMetricsImplementation::MediaLoadStart(const Exchange::IFbMetrics::Context &context,
                                                              const string &entityId)
        {
            LOGINFO("appId=%s] MediaLoadStart: entityId=%s",
                    context.appId.c_str(), entityId.c_str());

            string eventName = "inapp_media";

            // "eventPayload": {
            //     "media_event_name": "mediaLoadStart",
            //     "src_entity_id": "<entityId>",
            //     "app_session_id": "<from update_app_context>",
            //     "app_user_session_id": "<empty string if None>",
            //     "durable_app_id": "<ctx.app_id>",
            //     "app_version": "<from metrics.appInfo (optional)>"
            //   }

            JsonObject eventPayload;
            eventPayload["media_event_name"] = "mediaLoadStart";
            eventPayload["src_entity_id"] = entityId;
            UpdateWithAppContext(context, eventPayload);

            string eventPayloadStr;
            eventPayload.ToString(eventPayloadStr);

            return SendAnalyticsEvent(eventName, eventPayloadStr, context.appId);
        }

        Core::hresult FbMetricsImplementation::MediaPause(const Exchange::IFbMetrics::Context &context,
                                                          const string &entityId)
        {
            LOGINFO("appId=%s] MediaPause: entityId=%s",
                    context.appId.c_str(), entityId.c_str());

            string eventName = "inapp_media";
            // "eventPayload" : {
            //     "media_event_name" : "mediaPause",
            //     "src_entity_id" : "<entityId>",
            //     "app_session_id" : "<from update_app_context>",
            //     "app_user_session_id" : "<empty string if None>",
            //     "durable_app_id" : "<ctx.app_id>",
            //     "app_version" : "<from metrics.appInfo (optional)>"
            // }

            JsonObject eventPayload;
            eventPayload["media_event_name"] = "mediaPause";
            eventPayload["src_entity_id"] = entityId;
            UpdateWithAppContext(context, eventPayload);

            string eventPayloadStr;
            eventPayload.ToString(eventPayloadStr);

            return SendAnalyticsEvent(eventName, eventPayloadStr, context.appId);
        }

        Core::hresult FbMetricsImplementation::MediaPlay(const Exchange::IFbMetrics::Context &context,
                                                         const string &entityId)
        {
            LOGINFO("appId=%s] MediaPlay: entityId=%s",
                   context.appId.c_str(), entityId.c_str());

            string eventName = "inapp_media";

            // "eventPayload" : {
            //     "media_event_name" : "mediaPlay",
            //     "src_entity_id" : "<entityId>",
            //     "app_session_id" : "<from update_app_context>",
            //     "app_user_session_id" : "<empty string if None>",
            //     "durable_app_id" : "<ctx.app_id>",
            //     "app_version" : "<from metrics.appInfo (optional)>"
            // }

            JsonObject eventPayload;
            eventPayload["media_event_name"] = "mediaPlay";
            eventPayload["src_entity_id"] = entityId;
            UpdateWithAppContext(context, eventPayload);

            string eventPayloadStr;
            eventPayload.ToString(eventPayloadStr);

            return SendAnalyticsEvent(eventName, eventPayloadStr, context.appId);
        }

        Core::hresult FbMetricsImplementation::MediaPlaying(const Exchange::IFbMetrics::Context &context,
                                                            const string &entityId)
        {
            LOGINFO("appId=%s] MediaPlaying: entityId=%s",
                    context.appId.c_str(), entityId.c_str());

            string eventName = "inapp_media";

            // "eventPayload" : {
            //     "media_event_name" : "mediaPlaying",
            //     "src_entity_id" : "<entityId>",
            //     "app_session_id" : "<from update_app_context>",
            //     "app_user_session_id" : "<empty string if None>",
            //     "durable_app_id" : "<ctx.app_id>",
            //     "app_version" : "<optional>"
            // }

            JsonObject eventPayload;
            eventPayload["media_event_name"] = "mediaPlaying";
            eventPayload["src_entity_id"] = entityId;
            UpdateWithAppContext(context, eventPayload);

            string eventPayloadStr;
            eventPayload.ToString(eventPayloadStr);

            return SendAnalyticsEvent(eventName, eventPayloadStr, context.appId);
        }

        Core::hresult FbMetricsImplementation::MediaProgress(const Exchange::IFbMetrics::Context &context,
                                                             const string &entityId,
                                                             const string &progress)
        {
            LOGINFO("appId=%s] MediaProgress: entityId=%s, progress=%s",
                    context.appId.c_str(), entityId.c_str(), progress.c_str());

            // TODO: Sending to discovery is Sky specifics, will be handled in the future
            //  if self.state.device_manifest.configuration.default_values.media_progress_as_watched_events
            //     && progress != MediaPositionType::None
            //  {
            //      let request = WatchedRequest {
            //          context: ctx.clone(),
            //          info: WatchedInfo {
            //              entity_id: media_progress_params.entity_id.clone(),
            //              progress: p,                   // original f32 you passed in
            //              completed: None,
            //              watched_on: None,
            //          },
            //          unit: None,
            //      };
            //      let discovery = DiscoveryImpl::new(&self.state);
            //      if discovery.handle_watched(request).await {
            //          error!("Error sending watched event");
            //      }
            //  }

            string eventName = "inapp_media";
            // "eventPayload" : {
            //     "media_event_name" : "mediaProgress",
            //     "src_entity_id" : "<entityId>",
            //     "app_session_id" : "<from update_app_context>",
            //     "app_user_session_id" : "<empty string if None>",
            //     "durable_app_id" : "<ctx.app_id>",
            //     "app_version" : "<from metrics.appInfo (optional)>",

            //     // Exactly one of the following appears, depending on your input:
            //     "media_pos_pct" : 42,      // integer percent, if input was 0.0..=0.999
            //     "media_pos_seconds" : 1234 // integer seconds (1..=86400), if input was absolute
            // }
            JsonObject eventPayload;
            eventPayload["media_event_name"] = "mediaProgress";
            eventPayload["src_entity_id"] = entityId;
            UpdateWithAppContext(context, eventPayload);

            // Parse progress and add to event payload,  string to double conversion

            double progressValue;
            try
            {
                progressValue = std::stod(progress);
            }
            catch (const std::invalid_argument &e)
            {
                LOGERR("Invalid progress value: %s", e.what());
                return Core::ERROR_BAD_REQUEST;
            }
            catch (const std::out_of_range &e)
            {
                LOGERR("Progress value out of range: %s", e.what());
                return Core::ERROR_BAD_REQUEST;
            }

            if (progressValue >= 0.0 && progressValue <= 1.0)
            {
                eventPayload["media_pos_pct"] = static_cast<uint32_t>(progressValue * 100);
            }
            else
            {
                eventPayload["media_pos_seconds"] = static_cast<uint32_t>(progressValue);
            }

            string eventPayloadStr;
            eventPayload.ToString(eventPayloadStr);

            return SendAnalyticsEvent(eventName, eventPayloadStr, context.appId);
        }

        Core::hresult FbMetricsImplementation::MediaRateChange(const Exchange::IFbMetrics::Context &context,
                                                               const string &entityId,
                                                               const double rate)
        {
            LOGINFO("appId=%s] MediaRateChange: entityId=%s, rate=%f",
                    context.appId.c_str(), entityId.c_str(), rate);

            string eventName = "inapp_media";

            // "eventPayload" : {
            //     "media_event_name" : "mediaRateChange",
            //     "src_entity_id" : "<entityId>",
            //     "app_session_id" : "<from update_app_context>",
            //     "app_user_session_id" : "<empty string if None>",
            //     "durable_app_id" : "<ctx.app_id>",
            //     "app_version" : "<from metrics.appInfo (optional)>",
            //     "playback_rate" : 2 // integer rate from params.rate
            // }

            JsonObject eventPayload;
            eventPayload["media_event_name"] = "mediaRateChange";
            eventPayload["src_entity_id"] = entityId;
            UpdateWithAppContext(context, eventPayload);
            eventPayload["playback_rate"] = static_cast<int>(rate); // Convert double to int

            string eventPayloadStr;
            eventPayload.ToString(eventPayloadStr);

            return SendAnalyticsEvent(eventName, eventPayloadStr, context.appId);
        }

        Core::hresult FbMetricsImplementation::MediaRenditionChange(const Exchange::IFbMetrics::Context &context,
                                                                    const string &entityId,
                                                                    const uint32_t bitrate,
                                                                    const uint32_t width,
                                                                    const uint32_t height,
                                                                    const string &profile)
        {
            LOGINFO("appId=%s] MediaRenditionChange: entityId=%s, bitrate=%u, width=%u, height=%u, profile=%s",
                    context.appId.c_str(), entityId.c_str(), bitrate, width, height, profile.c_str());

            string eventName = "inapp_media";
            // "eventPayload" : {
            //     "media_event_name" : "mediaRenditionChange",
            //     "src_entity_id" : "<entityId>",
            //     "app_session_id" : "<from update_app_context>",
            //     "app_user_session_id" : "<empty string if None>",
            //     "durable_app_id" : "<ctx.app_id>",
            //     "app_version" : "<from metrics.appInfo (optional)>",

            //     "playback_bitrate" : <bitrate>,  // from params.bitrate (u32 → integer)
            //     "playback_width" : <width>,      // from params.width   (u32)
            //     "playback_height" : <height>,    // from params.height  (u32)
            //     "playback_profile" : "<profile>" // from params.profile (optional)
            // }

            JsonObject eventPayload;
            eventPayload["media_event_name"] = "mediaRenditionChange";
            eventPayload["src_entity_id"] = entityId;
            UpdateWithAppContext(context, eventPayload);
            eventPayload["playback_bitrate"] = bitrate;
            eventPayload["playback_width"] = width;
            eventPayload["playback_height"] = height;
            if (!profile.empty())
            {
                eventPayload["playback_profile"] = profile;
            }

            string eventPayloadStr;
            eventPayload.ToString(eventPayloadStr);

            return SendAnalyticsEvent(eventName, eventPayloadStr, context.appId);
        }

        Core::hresult FbMetricsImplementation::MediaSeeked(const Exchange::IFbMetrics::Context &context,
                                                           const string &entityId,
                                                           const string &position)
        {
            LOGINFO("appId=%s] MediaSeeked: entityId=%s, position=%s",
                    context.appId.c_str(), entityId.c_str(), position.c_str());
            string eventName = "inapp_media";

            // "eventPayload" : {
            //     "media_event_name" : "mediaSeeked",
            //     "src_entity_id" : "<entityId>",
            //     "app_session_id" : "<from update_app_context>",
            //     "app_user_session_id" : "<empty string if None>",
            //     "durable_app_id" : "<ctx.app_id>",
            //     "app_version" : "<from metrics.appInfo (optional)>",

            //     // exactly one of the following appears (or neither if invalid input was coerced to None):
            //     "media_pos_pct" : 42,      // integer percent, if input was 0.0..=0.999
            //     "media_pos_seconds" : 1234 // integer seconds (1..=86400), if input was absolute
            // }

            JsonObject eventPayload;
            eventPayload["media_event_name"] = "mediaSeeked";
            eventPayload["src_entity_id"] = entityId;
            UpdateWithAppContext(context, eventPayload);

            // Parse position and add to event payload, string to double conversion
            double positionValue;
            try
            {
                positionValue = std::stod(position);
            }
            catch (const std::invalid_argument &e)
            {
                LOGERR("Invalid position value: %s", e.what());
                return Core::ERROR_BAD_REQUEST;
            }
            catch (const std::out_of_range &e)
            {
                LOGERR("Position value out of range: %s", e.what());
                return Core::ERROR_BAD_REQUEST;
            }

            if (positionValue >= 0.0 && positionValue <= 1.0)
            {
                eventPayload["media_pos_pct"] = static_cast<uint32_t>(positionValue * 100);
            }
            else
            {
                eventPayload["media_pos_seconds"] = static_cast<uint32_t>(positionValue);
            }

            string eventPayloadStr;
            eventPayload.ToString(eventPayloadStr);

            return SendAnalyticsEvent(eventName, eventPayloadStr, context.appId);
        }

        Core::hresult FbMetricsImplementation::MediaSeeking(const Exchange::IFbMetrics::Context &context,
                                                            const string &entityId,
                                                            const string &target)
        {
            LOGINFO("appId=%s] MediaSeeking: entityId=%s, target=%s",
                    context.appId.c_str(), entityId.c_str(), target.c_str());

            string eventName = "inapp_media";
            // "eventPayload" : {
            //     "media_event_name" : "mediaSeeking",
            //     "src_entity_id" : "<entityId>",
            //     "app_session_id" : "<from update_app_context>",
            //     "app_user_session_id" : "<empty string if None>",
            //     "durable_app_id" : "<ctx.app_id>",
            //     "app_version" : "<from metrics.appInfo (optional)>",

            //     // exactly one of these (based on validated input):
            //     "media_pos_pct" : 42,      // integer percent, if target was 0.0..=0.999
            //     "media_pos_seconds" : 1234 // integer seconds (1..=86400), if target was absolute
            // }

            JsonObject eventPayload;
            eventPayload["media_event_name"] = "mediaSeeking";
            eventPayload["src_entity_id"] = entityId;
            UpdateWithAppContext(context, eventPayload);

            // Parse target and add to event payload, string to double conversion
            double targetValue;
            try
            {
                targetValue = std::stod(target);
            }
            catch (const std::invalid_argument &e)
            {
                LOGERR("Invalid target value: %s", e.what());
                return Core::ERROR_BAD_REQUEST;
            }
            catch (const std::out_of_range &e)
            {
                LOGERR("Target value out of range: %s", e.what());
                return Core::ERROR_BAD_REQUEST;
            }

            if (targetValue >= 0.0 && targetValue <= 1.0)
            {
                eventPayload["media_pos_pct"] = static_cast<uint32_t>(targetValue * 100);
            }
            else
            {
                eventPayload["media_pos_seconds"] = static_cast<uint32_t>(targetValue);
            }

            string eventPayloadStr;
            eventPayload.ToString(eventPayloadStr);

            return SendAnalyticsEvent(eventName, eventPayloadStr, context.appId);
        }
        Core::hresult FbMetricsImplementation::MediaWaiting(const Exchange::IFbMetrics::Context &context,
                                                            const string &entityId)
        {
            LOGINFO("appId=%s] MediaWaiting: entityId=%s",
                    context.appId.c_str(), entityId.c_str());

            string eventName = "inapp_media";
            // "eventPayload" : {
            //     "media_event_name" : "mediaWaiting",
            //     "src_entity_id" : "<entityId>",
            //     "app_session_id" : "<from update_app_context>",
            //     "app_user_session_id" : "<empty string if None>",
            //     "durable_app_id" : "<ctx.app_id>",
            //     "app_version" : "<optional>"
            // }

            JsonObject eventPayload;
            eventPayload["media_event_name"] = "mediaWaiting";
            eventPayload["src_entity_id"] = entityId;
            UpdateWithAppContext(context, eventPayload);

            string eventPayloadStr;
            eventPayload.ToString(eventPayloadStr);

            return SendAnalyticsEvent(eventName, eventPayloadStr, context.appId);
        }

        Core::hresult FbMetricsImplementation::Page(const Exchange::IFbMetrics::Context &context,
                                                    const string &pageId)
        {
            LOGINFO("appId=%s] Page: pageId=%s",
                    context.appId.c_str(), pageId.c_str());

            string eventName = "inapp_page_view";
            // "eventPayload" : {
            //     "src_page_id" : "<pageId>",
            //     "app_session_id" : "<from update_app_context>",
            //     "app_user_session_id" : "<empty string if None>",
            //     "durable_app_id" : "<ctx.app_id>",
            //     "app_version" : "<from metrics.appInfo (optional)>"
            // }

            JsonObject eventPayload;
            eventPayload["src_page_id"] = pageId;
            UpdateWithAppContext(context, eventPayload);

            string eventPayloadStr;
            eventPayload.ToString(eventPayloadStr);

            return SendAnalyticsEvent(eventName, eventPayloadStr, context.appId);
        }

        Core::hresult FbMetricsImplementation::StartContent(const Exchange::IFbMetrics::Context &context,
                                                            const string &entityId)
        {
            LOGINFO("appId=%s] StartContent: entityId=%s",
                    context.appId.c_str(), entityId.c_str());

            string eventName = "inapp_content_start";

            // "eventPayload" : {
            //     "src_entity_id" : "<entityId or omitted if none>",
            //     "app_session_id" : "<from update_app_context>",
            //     "app_user_session_id" : "<empty string if None>",
            //     "durable_app_id" : "<ctx.app_id>",
            //     "app_version" : "<from metrics.appInfo (optional)>"
            // }

            JsonObject eventPayload;
            if (!entityId.empty())
            {
                eventPayload["src_entity_id"] = entityId;
            }
            UpdateWithAppContext(context, eventPayload);

            string eventPayloadStr;
            eventPayload.ToString(eventPayloadStr);
            return SendAnalyticsEvent(eventName, eventPayloadStr, context.appId);
        }

        Core::hresult FbMetricsImplementation::StopContent(const Exchange::IFbMetrics::Context &context,
                                                           const string &entityId)
        {
            LOGINFO("appId=%s] StopContent: entityId=%s",
                    context.appId.c_str(), entityId.c_str());

            string eventName = "inapp_content_stop";
            // "eventPayload" : {
            //     "src_entity_id" : "<entityId or omitted if none>",
            //     "app_session_id" : "<from update_app_context>",
            //     "app_user_session_id" : "<empty string if None>",
            //     "durable_app_id" : "<ctx.app_id>",
            //     "app_version" : "<from metrics.appInfo (optional)>"
            // }

            JsonObject eventPayload;
            if (!entityId.empty())
            {
                eventPayload["src_entity_id"] = entityId;
            }
            UpdateWithAppContext(context, eventPayload);

            string eventPayloadStr;
            eventPayload.ToString(eventPayloadStr);
            return SendAnalyticsEvent(eventName, eventPayloadStr, context.appId);
        }

        Core::hresult FbMetricsImplementation::SignIn(const Context &context, const string &entitlements)
        {
            LOGINFO("appId=%s] Signing in with entitlements: %s",
                    context.appId.c_str(), entitlements.c_str());
            string eventName = "inapp_other_action";

            // "eventPayload": {
            //   "category": "user",
            //   "type": "sign_in",
            //   "app_session_id": "<from update_app_context>",
            //   "app_user_session_id": "<string; empty if not available>",
            //   "durable_app_id": "<ctx.app_id>",
            //   "app_version": "<optional; omitted if not set>"
            // }

            JsonObject eventPayload;
            eventPayload["category"] = "user";
            eventPayload["type"] = "sign_in";
            UpdateWithAppContext(context, eventPayload);

            // entitlements not handled

            string eventPayloadStr;
            eventPayload.ToString(eventPayloadStr);

            return SendAnalyticsEvent(eventName, eventPayloadStr, context.appId);
        }

        Core::hresult FbMetricsImplementation::SignOut(const Context &context)
        {
            LOGINFO("appId=%s] Signing out",
                    context.appId.c_str());
            string eventName = "inapp_other_action";

            // "eventPayload":
            //  {
            //   "category": "user",
            //   "type": "sign_out",
            //   "app_session_id": "<from update_app_context>",
            //   "app_user_session_id": "<empty string if none>",
            //   "durable_app_id": "<ctx.app_id>",
            //   "app_version": "<optional; omitted if not set>"
            // }

            JsonObject eventPayload;
            eventPayload["category"] = "user";
            eventPayload["type"] = "sign_out";
            UpdateWithAppContext(context, eventPayload);

            string eventPayloadStr;
            eventPayload.ToString(eventPayloadStr);

            return SendAnalyticsEvent(eventName, eventPayloadStr, context.appId);
        }

        Core::hresult FbMetricsImplementation::SetAppUserSessionId(const string& id)
        {
            LOGINFO("Setting app user session ID: %s", id.c_str());

            if (id.empty())
            {
                mAppUserSessionIdRegistry.Clear();
            }
            else
            {
                mAppUserSessionIdRegistry.Set(id);
            }

            return Core::ERROR_NONE;
        }

        Core::hresult FbMetricsImplementation::SetLifeCycle(const Context& context, const string&  newState, const string&  previousState)
        {
            LOGINFO("[appId=%s] Setting lifecycle: %s -> %s",
                    context.appId.c_str(), previousState.c_str(), newState.c_str());

            //  "eventPayload": {
            //   "app_session_id": "...",
            //   "app_user_session_id": "...",     // Omitted if None
            //   "durable_app_id": "...",
            //   "app_version": "...",             // Omitted if None
            //   "previous_life_cycle_state": "...", // Omitted if None
            //   "new_life_cycle_state": "..."
            // }

            string eventName = "app_lc_state_change";

            JsonObject eventPayload;
            if (!previousState.empty())
            {
                eventPayload["previous_life_cycle_state"] = previousState;
            }
            eventPayload["new_life_cycle_state"] = newState;
            UpdateWithAppContext(context, eventPayload);

            string eventPayloadStr;
            eventPayload.ToString(eventPayloadStr);

            return SendAnalyticsEvent(eventName, eventPayloadStr, context.appId);
        }

        uint32_t FbMetricsImplementation::Configure(PluginHost::IShell *shell)
        {
            LOGINFO("Configuring FbMetrics");
            uint32_t result = Core::ERROR_NONE;
            ASSERT(shell != nullptr);
            mShell = shell;
            mShell->AddRef();
            return result;
        }

        std::string FbMetricsImplementation::GetAppSessionId(const string &appId)
        {
            string sessionId = "app_session_id.not.set";
            if (mShell != nullptr)
            {
                auto launchDelegate = mShell->QueryInterfaceByCallsign<Exchange::IAppGatewayAuthenticatorInternal>(INTERNAL_GATEWAY_CALLSIGN);
                if (launchDelegate != nullptr)
                {
                    uint32_t err = launchDelegate->GetSessionId(appId, sessionId);
                    launchDelegate->Release();

                    if (err != Core::ERROR_NONE || sessionId.empty())
                    {
                        LOGERR("Failed to get app session ID for appId: %s, error: %d", appId.c_str(), err);
                    }
                }
                else
                {
                    LOGERR("LaunchDelegate interface not found.");
                }
            }
            else
            {
                LOGERR("Shell is not initialized.");
            }
            return sessionId;
        }

        Core::hresult FbMetricsImplementation::SendAnalyticsEvent(const string &eventName, const string &payload, const string &appId)
        {
            LOGINFO("Sending analytics event: %s", eventName.c_str());

            if (mShell == nullptr)
            {
                LOGERR("Shell is not initialized.");
                return Core::ERROR_UNAVAILABLE;
            }

            // cetList is added by Analytics, here only empty one is created
            // TODO: 
            // 1. handle nullptr in Analytics greacefully
            // 2. Add new cet if required here
            // 3. Modify Analytics to only append to provided cetList
            WPEFramework::Exchange::IAnalytics::IStringIterator *cetList = (Core::Service<RPC::StringIterator>::Create<RPC::IStringIterator>(std::list<string>()));
            if (cetList == nullptr)
            {
                LOGERR("Failed to create empty IStringIterator.");
                return Core::ERROR_UNAVAILABLE;
            }

            string eventVersion = FBMETRICS_EVENT_VERSION_DFL;
            string eventSource = FBMETRICS_EVENT_SOURCE_DFL;
            string eventSourceVersion = FBMETRICS_EVENT_SOURCE_VERSION_DFL;

            auto analyticsInterface = mShell->QueryInterfaceByCallsign<Exchange::IAnalytics>(ANALYTICS_PLUGIN_CALLSIGN);
            if (analyticsInterface == nullptr)
            {
                LOGERR("Analytics interface not found.");
                return Core::ERROR_UNAVAILABLE;
            }

            Core::hresult result = analyticsInterface->SendEvent(eventName, eventVersion, eventSource,
                                                                 eventSourceVersion, cetList, 0, 0, appId, payload);

            analyticsInterface->Release();

            if (result != Core::ERROR_NONE)
            {
                LOGERR("Failed to send event: %d", result);
            }

            return result;
        }

        void FbMetricsImplementation::UpdateWithAppContext(const Exchange::IFbMetrics::Context &context,
                                                           JsonObject &eventPayload)
        {
            // Update the event payload with app session and user session IDs
            eventPayload["app_session_id"] = GetAppSessionId(context.appId);
            std::string appUserSessionId = mAppUserSessionIdRegistry.Get();
            if (!appUserSessionId.empty())
            {
                eventPayload["app_user_session_id"] = appUserSessionId;
            }
            else
            {
                LOGWARN("app_user_session_id is empty");
            }

            eventPayload["durable_app_id"] = context.appId;

            std::string appVersion;
            if (mAppVersionCache.Get(context.appId, appVersion))
            {
                eventPayload["app_version"] = appVersion;
            }
        }

    } // namespace Plugin
} // namespace WPEFramework
