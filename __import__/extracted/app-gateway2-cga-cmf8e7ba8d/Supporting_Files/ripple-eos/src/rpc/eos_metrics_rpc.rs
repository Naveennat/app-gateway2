use chrono::Utc;
use jsonrpsee::{
    core::{async_trait, RpcResult},
    proc_macros::rpc,
};
use ripple_sdk::api::firebolt::fb_discovery::AgePolicy;
use serde_json::Number;
use std::collections::HashMap;
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_lifecycle_management::SessionResponse;
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_metrics::AppLifecycleStateChange;
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_metrics::Param;
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_telemetry::AppSDKLoaded;
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_telemetry::TelemetryPayload;
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_telemetry::TelemetryUtil;
use thunder_ripple_sdk::ripple_sdk::utils::rpc_utils::rpc_err;
use thunder_ripple_sdk::ripple_sdk::{
    api::{
        account_link::WatchedRequest,
        firebolt::{
            fb_capabilities::JSON_RPC_STANDARD_ERROR_INVALID_PARAMS,
            fb_discovery::WatchedInfo,
            fb_metrics::{
                hashmap_to_param_vec, ErrorParams, FlatMapValue, InternalInitializeParams,
                InternalInitializeResponse, Version,
            },
        },
        gateway::rpc_gateway_api::CallContext,
    },
    log::{error, trace},
    utils::rpc_utils::rpc_error_with_code_result,
};

use serde::{Deserialize, Serialize};

use crate::{
    extn::eos_ffi::EOS_DISTRIBUTOR_SERVICE_ID,
    model::metrics::{
        Action, BadgerMetric, BadgerMetrics, BadgerMetricsService, BehavioralMetricContext,
        BehavioralMetricPayload, CategoryType, MediaEnded, MediaLoadStart, MediaPause, MediaPlay,
        MediaPlaying, MediaPositionType, MediaProgress, MediaRateChanged, MediaRenditionChanged,
        MediaSeeked, MediaSeeking, MediaWaiting, MetricsError, Page, Ready, SignIn, SignOut,
        StartContent, StopContent,
    },
    rpc::eos_discovery_rpc::DiscoveryImpl,
    state::{
        distributor_state::DistributorState,
        sift_metrics_state::{send_metric, update_app_context},
    },
};

const LAUNCH_COMPLETED_SEGMENT: &'static str = "LAUNCH_COMPLETED";

#[derive(Deserialize, Debug)]
pub struct PageParams {
    #[serde(rename = "pageId")]
    pub page_id: String,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}

#[derive(Debug, Deserialize, Clone)]
pub struct ActionParams {
    pub category: CategoryType,
    #[serde(rename = "type")]
    pub action_type: String,
    pub parameters: Option<HashMap<String, FlatMapValue>>,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}

#[derive(Deserialize, Debug, Clone)]
pub struct StartContentParams {
    #[serde(rename = "entityId")]
    pub entity_id: Option<String>,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}
#[derive(Deserialize, Debug, Clone)]
pub struct StopContentParams {
    #[serde(rename = "entityId")]
    pub entity_id: Option<String>,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}

fn validate_metrics_action_type(metrics_action: &str) -> RpcResult<bool> {
    match metrics_action.len() {
        1..=256 => Ok(true),
        _ => rpc_error_with_code_result(
            "metrics.action.action_type out of range".to_string(),
            JSON_RPC_STANDARD_ERROR_INVALID_PARAMS,
        ),
    }
}

pub const ERROR_MEDIA_POSITION_OUT_OF_RANGE: &str = "absolute media position out of range";
pub const ERROR_BAD_ABSOLUTE_MEDIA_POSITION: &str =
    "absolute media position must not contain any numbers to the right of the decimal point.";
/*
implement this: https://developer.comcast.com/firebolt-apis/core-sdk/v0.9.0/metrics#mediaposition
*/
fn convert_to_media_position_type(media_position: Option<f32>) -> RpcResult<MediaPositionType> {
    match media_position {
        Some(position) => {
            if (0.0..=0.999).contains(&position) {
                Ok(MediaPositionType::PercentageProgress(position))
            } else {
                if position.fract() != 0.0 {
                    return rpc_error_with_code_result(
                        ERROR_BAD_ABSOLUTE_MEDIA_POSITION.to_string(),
                        JSON_RPC_STANDARD_ERROR_INVALID_PARAMS,
                    );
                };
                let abs_position = position.round() as i32;

                if (1..=86400).contains(&abs_position) {
                    Ok(MediaPositionType::AbsolutePosition(abs_position))
                } else {
                    rpc_error_with_code_result(
                        ERROR_MEDIA_POSITION_OUT_OF_RANGE.to_string(),
                        JSON_RPC_STANDARD_ERROR_INVALID_PARAMS,
                    )
                }
            }
        }
        None => Ok(MediaPositionType::None),
    }
}

#[derive(Deserialize, Debug, Clone)]
pub struct MediaLoadStartParams {
    #[serde(rename = "entityId")]
    pub entity_id: String,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}
#[derive(Deserialize, Debug, Clone)]
pub struct MediaPlayParams {
    #[serde(rename = "entityId")]
    pub entity_id: String,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}
#[derive(Deserialize, Debug, Clone)]
pub struct MediaPlayingParams {
    #[serde(rename = "entityId")]
    pub entity_id: String,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}
#[derive(Deserialize, Debug, Clone)]
pub struct MediaPauseParams {
    #[serde(rename = "entityId")]
    pub entity_id: String,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}
#[derive(Deserialize, Debug, Clone)]
pub struct MediaWaitingParams {
    #[serde(rename = "entityId")]
    pub entity_id: String,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}
#[derive(Deserialize, Debug, Clone)]
pub struct MediaProgressParams {
    #[serde(rename = "entityId")]
    pub entity_id: String,
    pub progress: Option<f32>,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}
#[derive(Deserialize, Debug, Clone)]
pub struct MediaSeekingParams {
    #[serde(rename = "entityId")]
    pub entity_id: String,
    pub target: Option<f32>,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}
#[derive(Deserialize, Debug, Clone)]
pub struct MediaSeekedParams {
    #[serde(rename = "entityId")]
    pub entity_id: String,
    pub position: Option<f32>,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}
#[derive(Deserialize, Debug, Clone)]
pub struct MediaRateChangeParams {
    #[serde(rename = "entityId")]
    pub entity_id: String,
    pub rate: Number,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}
#[derive(Deserialize, Debug, Clone)]
pub struct MediaRenditionChangeParams {
    #[serde(rename = "entityId")]
    pub entity_id: String,
    pub bitrate: Number,
    pub width: u32,
    pub height: u32,
    pub profile: Option<String>,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}
#[derive(Deserialize, Debug, Clone)]
pub struct MediaEndedParams {
    #[serde(rename = "entityId")]
    pub entity_id: String,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}

#[derive(Deserialize, Debug, Clone)]
pub struct AppInfoParams {
    pub build: String,
}

#[derive(Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub enum HandlerParamEventType {
    UserAction,
    AppAction,
    PageView,
    Error,
    UserError,
}

#[derive(Deserialize, Debug, Clone)]
pub struct HandlerParamEvt {}

#[derive(Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct BadgerMetricsHandlerParams {
    pub segment: Option<String>,
    pub event_type: Option<HandlerParamEventType>,
    pub action: Option<String>,
    pub evt: Option<HandlerParamEvt>,
    pub page: Option<String>,
    pub err_msg: Option<String>,
    pub err_visible: Option<bool>,
    pub err_code: Option<String>,
    pub args: Option<Vec<Param>>,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}
#[derive(Default, Serialize, Debug)]
pub struct BadgerEmptyResult {
    //Empty object to take care of OTTX-28709
}

//https://developer.comcast.com/firebolt/core/sdk/latest/api/metrics
#[rpc(server)]
pub trait Metrics {
    #[method(name = "metrics.startContent")]
    async fn start_content(
        &self,
        ctx: CallContext,
        page_params: StartContentParams,
    ) -> RpcResult<bool>;
    #[method(name = "metrics.stopContent")]
    async fn stop_content(
        &self,
        ctx: CallContext,
        stop_content_params: StopContentParams,
    ) -> RpcResult<bool>;
    #[method(name = "metrics.page")]
    async fn page(&self, ctx: CallContext, page_params: PageParams) -> RpcResult<bool>;
    #[method(name = "metrics.action")]
    async fn action(&self, ctx: CallContext, action_params: ActionParams) -> RpcResult<bool>;
    #[method(name = "metrics.error")]
    async fn error(&self, ctx: CallContext, error_params: ErrorParams) -> RpcResult<bool>;
    #[method(name = "metrics.ready")]
    async fn ready(&self, ctx: CallContext) -> RpcResult<bool>;
    #[method(name = "metrics.signin", aliases=["metrics.signIn"])]
    async fn sign_in(&self, ctx: CallContext) -> RpcResult<bool>;
    #[method(name = "metrics.signout", aliases=["metrics.signOut"])]
    async fn sign_out(&self, ctx: CallContext) -> RpcResult<bool>;
    #[method(name = "internal.initialize")]
    async fn internal_initialize(
        &self,
        ctx: CallContext,
        internal_initialize_params: InternalInitializeParams,
    ) -> RpcResult<InternalInitializeResponse>;
    #[method(name = "metrics.mediaLoadStart")]
    async fn media_load_start(
        &self,
        ctx: CallContext,
        media_load_start_params: MediaLoadStartParams,
    ) -> RpcResult<bool>;
    #[method(name = "metrics.mediaPlay")]
    async fn media_play(
        &self,
        ctx: CallContext,
        media_play_params: MediaPlayParams,
    ) -> RpcResult<bool>;
    #[method(name = "metrics.mediaPlaying")]
    async fn media_playing(
        &self,
        ctx: CallContext,
        media_playing_params: MediaPlayingParams,
    ) -> RpcResult<bool>;
    #[method(name = "metrics.mediaPause")]
    async fn media_pause(
        &self,
        ctx: CallContext,
        media_pause_params: MediaPauseParams,
    ) -> RpcResult<bool>;
    #[method(name = "metrics.mediaWaiting")]
    async fn media_waiting(
        &self,
        ctx: CallContext,
        media_waiting_params: MediaWaitingParams,
    ) -> RpcResult<bool>;
    #[method(name = "metrics.mediaProgress")]
    async fn media_progress(
        &self,
        ctx: CallContext,
        media_progress_params: MediaProgressParams,
    ) -> RpcResult<bool>;
    #[method(name = "metrics.mediaSeeking")]
    async fn media_seeking(
        &self,
        ctx: CallContext,
        media_seeking_params: MediaSeekingParams,
    ) -> RpcResult<bool>;
    #[method(name = "metrics.mediaSeeked")]
    async fn media_seeked(
        &self,
        ctx: CallContext,
        media_seeked_params: MediaSeekedParams,
    ) -> RpcResult<bool>;
    #[method(name = "metrics.mediaRateChange")]
    async fn media_rate_change(
        &self,
        ctx: CallContext,
        media_rate_changed_params: MediaRateChangeParams,
    ) -> RpcResult<bool>;
    #[method(name = "metrics.mediaRenditionChange")]
    async fn media_rendition_change(
        &self,
        ctx: CallContext,
        media_rendition_change_params: MediaRenditionChangeParams,
    ) -> RpcResult<bool>;
    #[method(name = "metrics.mediaEnded")]
    async fn media_ended(
        &self,
        ctx: CallContext,
        media_ended_params: MediaEndedParams,
    ) -> RpcResult<bool>;
    #[method(name = "metrics.appInfo")]
    async fn app_info(&self, ctx: CallContext, app_info_params: AppInfoParams) -> RpcResult<()>;

    #[method(name = "eos.metricsHandler")]
    async fn badger_metrics_handler(
        &self,
        ctx: CallContext,
        badger_metrics_handler_params: BadgerMetricsHandlerParams,
    ) -> RpcResult<bool>;

    #[method(name = "eos.logMoneyBadgerLoaded")]
    async fn log(&self, ctx: CallContext) -> RpcResult<BadgerEmptyResult>;

    #[method(name = "ripple.reportLifecycleStateChange")]
    async fn ripple_lifecycle_update(
        &self,
        ctx: CallContext,
        state_change: AppLifecycleStateChange,
    ) -> RpcResult<()>;

    #[method(name = "ripple.reportSessionUpdate")]
    async fn ripple_session_id_update(
        &self,
        ctx: CallContext,
        session: SessionResponse,
    ) -> RpcResult<()>;
}

#[derive(Clone)]
pub struct MetricsImpl {
    state: DistributorState,
}

impl MetricsImpl {
    pub fn new(state: &DistributorState) -> Self {
        MetricsImpl {
            state: state.clone(),
        }
    }
}

impl From<ActionParams> for CategoryType {
    fn from(action_params: ActionParams) -> Self {
        action_params.category
    }
}

#[async_trait]
impl MetricsServer for MetricsImpl {
    async fn start_content(
        &self,
        ctx: CallContext,
        page_params: StartContentParams,
    ) -> RpcResult<bool> {
        let start_content = BehavioralMetricPayload::StartContent(StartContent {
            context: ctx.clone().into(),
            entity_id: page_params.entity_id,
            age_policy: page_params.age_policy,
        });

        trace!("metrics.startContent={:?}", start_content);
        match send_metric(&self.state, start_content, &ctx).await {
            Ok(_) => Ok(true),
            Err(_) => Err(rpc_err("parse error")),
        }
    }

    async fn stop_content(
        &self,
        ctx: CallContext,
        stop_content_params: StopContentParams,
    ) -> RpcResult<bool> {
        let stop_content = BehavioralMetricPayload::StopContent(StopContent {
            context: ctx.clone().into(),
            entity_id: stop_content_params.entity_id,
            age_policy: stop_content_params.age_policy,
        });
        trace!("metrics.stopContent={:?}", stop_content);
        match send_metric(&self.state, stop_content, &ctx).await {
            Ok(_) => Ok(true),
            Err(_) => Err(rpc_err("parse error")),
        }
    }
    async fn page(&self, ctx: CallContext, page_params: PageParams) -> RpcResult<bool> {
        let page = BehavioralMetricPayload::Page(Page {
            context: ctx.clone().into(),
            page_id: page_params.page_id,
            age_policy: page_params.age_policy,
        });
        trace!("metrics.page={:?}", page);
        match send_metric(&self.state, page, &ctx).await {
            Ok(_) => Ok(true),
            Err(_) => Err(rpc_err("parse error")),
        }
    }
    async fn action(&self, ctx: CallContext, action_params: ActionParams) -> RpcResult<bool> {
        let _ = validate_metrics_action_type(&action_params.action_type)?;
        let p_type = action_params.clone();

        let action = BehavioralMetricPayload::Action(Action {
            context: ctx.clone().into(),
            category: action_params.into(),
            parameters: hashmap_to_param_vec(p_type.parameters),
            _type: p_type.action_type,
            age_policy: p_type.age_policy,
        });
        trace!("metrics.action={:?}", action);

        match send_metric(&self.state, action, &ctx).await {
            Ok(_) => Ok(true),
            Err(_) => Err(rpc_err("parse error")),
        }
    }
    async fn ready(&self, ctx: CallContext) -> RpcResult<bool> {
        let data = BehavioralMetricPayload::Ready(Ready {
            context: BehavioralMetricContext::from(ctx.clone()),
            ttmu_ms: 12,
        });
        trace!("metrics.action = {:?}", data);
        match send_metric(&self.state, data, &ctx).await {
            Ok(_) => Ok(true),
            Err(_) => Err(rpc_err("parse error")),
        }
    }

    async fn error(&self, ctx: CallContext, error_params: ErrorParams) -> RpcResult<bool> {
        let app_id = ctx.app_id.clone();
        let error_message = BehavioralMetricPayload::Error(MetricsError {
            context: ctx.clone().into(),
            error_type: error_params.clone().into(),
            code: error_params.code.clone(),
            description: error_params.description.clone(),
            visible: error_params.visible,
            parameters: error_params.parameters.clone(),
            durable_app_id: app_id.clone(),
            third_party_error: true,
            age_policy: error_params.age_policy.clone(),
        });
        trace!("metrics.error={:?}", error_message);
        let ripple_session_id = self.state.metrics.get_context().device_session_id.clone();

        let service_client = match self.state.get_service_client() {
            Some(client) => client,
            None => return Err(rpc_err("error failed to get service client")),
        };

        TelemetryUtil::send_error(
            &service_client,
            &ctx.app_id,
            error_params.clone(),
            ripple_session_id,
        );
        match send_metric(&self.state, error_message, &ctx).await {
            Ok(_) => Ok(true),
            Err(_) => Err(rpc_err("parse error")),
        }
    }
    async fn sign_in(&self, ctx: CallContext) -> RpcResult<bool> {
        let data = BehavioralMetricPayload::SignIn(SignIn {
            context: ctx.clone().into(),
        });
        trace!("metrics.action = {:?}", data);
        Ok(send_metric(&self.state, data, &ctx).await.is_ok())
    }

    async fn sign_out(&self, ctx: CallContext) -> RpcResult<bool> {
        let data = BehavioralMetricPayload::SignOut(SignOut {
            context: ctx.clone().into(),
        });
        trace!("metrics.action = {:?}", data);
        Ok(send_metric(&self.state, data, &ctx).await.is_ok())
    }

    async fn internal_initialize(
        &self,
        ctx: CallContext,
        internal_initialize_params: InternalInitializeParams,
    ) -> RpcResult<InternalInitializeResponse> {
        let ripple_session_id = self.state.metrics.get_context().device_session_id.clone();
        let service_client = match self.state.get_service_client() {
            Some(client) => client,
            None => return Err(rpc_err("internal_initialize failed to get service client")),
        };

        TelemetryUtil::send_initialize(
            &service_client,
            &ctx,
            &internal_initialize_params,
            ripple_session_id,
        );
        let readable_result = internal_initialize_params
            .version
            .readable
            .replace("SDK", "FEE");
        let internal_initialize_resp = Version {
            major: internal_initialize_params.version.major,
            minor: internal_initialize_params.version.minor,
            patch: internal_initialize_params.version.patch,
            readable: readable_result,
        };
        Ok(InternalInitializeResponse {
            version: internal_initialize_resp,
        })
    }
    async fn media_load_start(
        &self,
        ctx: CallContext,
        media_load_start_params: MediaLoadStartParams,
    ) -> RpcResult<bool> {
        let media_load_start_message = BehavioralMetricPayload::MediaLoadStart(MediaLoadStart {
            context: ctx.clone().into(),
            entity_id: media_load_start_params.entity_id,
            age_policy: media_load_start_params.age_policy,
        });
        trace!("metrics.media_load_start={:?}", media_load_start_message);
        match send_metric(&self.state, media_load_start_message, &ctx).await {
            Ok(_) => Ok(true),
            Err(_) => Err(rpc_err("parse error")),
        }
    }
    async fn media_play(
        &self,
        ctx: CallContext,
        media_play_params: MediaPlayParams,
    ) -> RpcResult<bool> {
        let media_play_message = BehavioralMetricPayload::MediaPlay(MediaPlay {
            context: ctx.clone().into(),
            entity_id: media_play_params.entity_id,
            age_policy: media_play_params.age_policy,
        });
        trace!("metrics.media_play={:?}", media_play_message);
        match send_metric(&self.state, media_play_message, &ctx).await {
            Ok(_) => Ok(true),
            Err(_) => Err(rpc_err("parse error")),
        }
    }
    async fn media_playing(
        &self,
        ctx: CallContext,
        media_playing_params: MediaPlayingParams,
    ) -> RpcResult<bool> {
        let media_playing = BehavioralMetricPayload::MediaPlaying(MediaPlaying {
            context: ctx.clone().into(),
            entity_id: media_playing_params.entity_id,
            age_policy: media_playing_params.age_policy,
        });
        trace!("metrics.media_playing={:?}", media_playing);
        match send_metric(&self.state, media_playing, &ctx).await {
            Ok(_) => Ok(true),
            Err(_) => Err(rpc_err("parse error")),
        }
    }
    async fn media_pause(
        &self,
        ctx: CallContext,
        media_pause_params: MediaPauseParams,
    ) -> RpcResult<bool> {
        let media_pause = BehavioralMetricPayload::MediaPause(MediaPause {
            context: ctx.clone().into(),
            entity_id: media_pause_params.entity_id,
            age_policy: media_pause_params.age_policy,
        });
        trace!("metrics.media_pause={:?}", media_pause);
        match send_metric(&self.state, media_pause, &ctx).await {
            Ok(_) => Ok(true),
            Err(_) => Err(rpc_err("parse error")),
        }
    }
    async fn media_waiting(
        &self,
        ctx: CallContext,
        media_waiting_params: MediaWaitingParams,
    ) -> RpcResult<bool> {
        let media_waiting = BehavioralMetricPayload::MediaWaiting(MediaWaiting {
            context: ctx.clone().into(),
            entity_id: media_waiting_params.entity_id,
            age_policy: media_waiting_params.age_policy,
        });
        trace!("metrics.media_waiting={:?}", media_waiting);
        match send_metric(&self.state, media_waiting, &ctx).await {
            Ok(_) => Ok(true),
            Err(_) => Err(rpc_err("parse error")),
        }
    }
    async fn media_progress(
        &self,
        ctx: CallContext,
        media_progress_params: MediaProgressParams,
    ) -> RpcResult<bool> {
        let progress = convert_to_media_position_type(media_progress_params.progress)?;

        if self
            .state
            .device_manifest
            .configuration
            .default_values
            .media_progress_as_watched_events
            && progress != MediaPositionType::None
        {
            if let Some(p) = media_progress_params.progress {
                let request = WatchedRequest {
                    context: ctx.clone(),
                    info: WatchedInfo {
                        entity_id: media_progress_params.entity_id.clone(),
                        progress: p,
                        completed: None,
                        watched_on: None,
                        age_policy: media_progress_params.age_policy.clone(),
                    },
                    unit: None,
                };
                let discovery = DiscoveryImpl::new(&self.state);
                if discovery.handle_watched(request).await {
                    error!("Error sending watched event");
                }
            }
        }

        let media_progress = BehavioralMetricPayload::MediaProgress(MediaProgress {
            context: ctx.clone().into(),
            entity_id: media_progress_params.entity_id,
            progress: Some(progress),
            age_policy: media_progress_params.age_policy,
        });
        trace!("metrics.media_progress={:?}", media_progress);
        match send_metric(&self.state, media_progress, &ctx).await {
            Ok(_) => Ok(true),
            Err(_) => Err(rpc_err("parse error")),
        }
    }
    async fn media_seeking(
        &self,
        ctx: CallContext,
        media_seeking_params: MediaSeekingParams,
    ) -> RpcResult<bool> {
        let target = convert_to_media_position_type(media_seeking_params.target)?;

        let media_seeking = BehavioralMetricPayload::MediaSeeking(MediaSeeking {
            context: ctx.clone().into(),
            entity_id: media_seeking_params.entity_id,
            target: Some(target),
            age_policy: media_seeking_params.age_policy,
        });
        trace!("metrics.media_seeking={:?}", media_seeking);
        match send_metric(&self.state, media_seeking, &ctx).await {
            Ok(_) => Ok(true),
            Err(_) => Err(rpc_err("parse error")),
        }
    }
    async fn media_seeked(
        &self,
        ctx: CallContext,
        media_seeked_params: MediaSeekedParams,
    ) -> RpcResult<bool> {
        let position = convert_to_media_position_type(media_seeked_params.position)
            .unwrap_or(MediaPositionType::None);
        let media_seeked = BehavioralMetricPayload::MediaSeeked(MediaSeeked {
            context: ctx.clone().into(),
            entity_id: media_seeked_params.entity_id,
            position: Some(position),
            age_policy: media_seeked_params.age_policy,
        });
        trace!("metrics.media_seeked={:?}", media_seeked);
        match send_metric(&self.state, media_seeked, &ctx).await {
            Ok(_) => Ok(true),
            Err(_) => Err(rpc_err("parse error")),
        }
    }
    async fn media_rate_change(
        &self,
        ctx: CallContext,
        media_rate_changed_params: MediaRateChangeParams,
    ) -> RpcResult<bool> {
        let media_rate_change = BehavioralMetricPayload::MediaRateChanged(MediaRateChanged {
            context: ctx.clone().into(),
            entity_id: media_rate_changed_params.entity_id,
            rate: media_rate_changed_params.rate,
            age_policy: media_rate_changed_params.age_policy,
        });
        trace!("metrics.media_seeked={:?}", media_rate_change);
        match send_metric(&self.state, media_rate_change, &ctx).await {
            Ok(_) => Ok(true),
            Err(_) => Err(rpc_err("parse error")),
        }
    }
    async fn media_rendition_change(
        &self,
        ctx: CallContext,
        media_rendition_change_params: MediaRenditionChangeParams,
    ) -> RpcResult<bool> {
        let media_rendition_change =
            BehavioralMetricPayload::MediaRenditionChanged(MediaRenditionChanged {
                context: ctx.clone().into(),
                entity_id: media_rendition_change_params.entity_id,
                bitrate: media_rendition_change_params.bitrate,
                height: media_rendition_change_params.height,
                profile: media_rendition_change_params.profile,
                width: media_rendition_change_params.width,
                age_policy: media_rendition_change_params.age_policy,
            });
        trace!(
            "metrics.media_rendition_change={:?}",
            media_rendition_change
        );
        match send_metric(&self.state, media_rendition_change, &ctx).await {
            Ok(_) => Ok(true),
            Err(_) => Err(rpc_err("parse error")),
        }
    }
    async fn media_ended(
        &self,
        ctx: CallContext,
        media_ended_params: MediaEndedParams,
    ) -> RpcResult<bool> {
        let media_ended = BehavioralMetricPayload::MediaEnded(MediaEnded {
            context: ctx.clone().into(),
            entity_id: media_ended_params.entity_id,
            age_policy: media_ended_params.age_policy,
        });
        trace!("metrics.media_ended={:?}", media_ended);

        match send_metric(&self.state, media_ended, &ctx).await {
            Ok(_) => Ok(true),
            Err(_) => Err(rpc_err("parse error")),
        }
    }
    async fn app_info(&self, ctx: CallContext, app_info_params: AppInfoParams) -> RpcResult<()> {
        trace!("metrics.app_info: app_info_params={:?}", app_info_params);
        match self
            .state
            .metrics
            .set_app_metrics_version(&ctx.app_id, app_info_params.build)
        {
            Ok(_) => Ok(()),
            Err(e) => {
                error!("Error setting app metrics version: {:?}", e);
                Err(rpc_err("Unable to set app info"))
            }
        }
    }

    async fn badger_metrics_handler(
        &self,
        ctx: CallContext,
        badger_metrics_params: BadgerMetricsHandlerParams,
    ) -> RpcResult<bool> {
        let client = match self.state.get_service_client() {
            Some(client) => client,
            None => {
                return Err(rpc_err(
                    "badger_metrics_handler failed to get service client",
                ))
            }
        };

        let mut ready = false;
        if let Some(v) = &badger_metrics_params.segment {
            if v.clone().eq(LAUNCH_COMPLETED_SEGMENT) {
                ready = true;
            }
        }

        let metric = BadgerMetric {
            context: ctx.clone().into(),
            segment: badger_metrics_params.segment.to_owned(),
            args: badger_metrics_params.args.clone(),
        };

        let mut launch_completed = BadgerMetrics::Metric(metric);
        update_app_context(&self.state, &ctx, &mut launch_completed).await;

        self.state
            .metrics
            .service
            .send_badger_metric(launch_completed)
            .await;
        if ready {
            let method = "lifecycle.ready".to_string();
            let ctx = Some(&ctx);
            let _ =
                client.request_transient(method, None, ctx, EOS_DISTRIBUTOR_SERVICE_ID.to_string());
        }

        Ok(true)
    }

    async fn log(&self, ctx: CallContext) -> RpcResult<BadgerEmptyResult> {
        let mut metrics = BadgerMetrics::Metric(BadgerMetric {
            context: ctx.clone().into(),
            segment: Some("moneyBadgerLoaded".to_string()),
            args: None,
        });
        update_app_context(&self.state, &ctx, &mut metrics).await;
        self.state.metrics.service.send_badger_metric(metrics).await;
        let ripple_session_id = self.state.metrics.get_context().device_session_id.clone();
        let service_client = match self.state.get_service_client() {
            Some(client) => client,
            None => return Err(rpc_err("MetricsImpl::log failed to get service client")),
        };

        TelemetryUtil::send_telemetry(
            &service_client,
            TelemetryPayload::AppSDKLoaded(AppSDKLoaded {
                app_id: ctx.app_id.to_owned(),
                stop_time: Utc::now().timestamp_millis(),
                ripple_session_id,
                app_session_id: None,
                sdk_name: "money_badger".into(),
            }),
        );

        Ok(BadgerEmptyResult::default())
    }

    async fn ripple_lifecycle_update(
        &self,
        ctx: CallContext,
        state_change: AppLifecycleStateChange,
    ) -> RpcResult<()> {
        println!("inside lc update");
        let state_change = BehavioralMetricPayload::AppStateChange(state_change.into());
        match send_metric(&self.state, state_change, &ctx).await {
            Ok(_) => Ok(()),
            Err(_) => Err(rpc_err("parse error")),
        }
    }

    async fn ripple_session_id_update(
        &self,
        _ctx: CallContext,
        session: SessionResponse,
    ) -> RpcResult<()> {
        if let SessionResponse::Completed(c) = session {
            self.state.metrics.set_app_session(c);
        }
        Ok(())
    }
}
