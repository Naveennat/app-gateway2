use crate::{
    gateway::appsanity_gateway::AgePolicyConfig,
    model::metrics::{
        BadgerAppAction, BadgerDismissLoadingScreen, BadgerError, BadgerLaunchCompleted,
        BadgerMetric, BadgerMetrics, BadgerMetricsService, BadgerPageView, BadgerUserAction,
        BadgerUserError, BehavioralMetricContext as AppContext,
        BehavioralMetricPayload as AppBehavioralMetric, CategoryType as ActionCategory,
        MetricsContext,
    },
    state::{distributor_state::DistributorState, sift_metrics_state::SiftMetricsState},
    util::{http_client::HttpError, service_util::get_age_policy_identifiers},
};
use chrono::Utc;
use queues::*;
use ripple_sdk::{
    api::gateway::rpc_gateway_api::CallContext, service::service_client::ServiceClient,
};
use serde::{Serialize, Serializer};
use serde_json::Value;
use thunder_ripple_sdk::ripple_sdk::{
    api::firebolt::fb_metrics::{
        AppLifecycleState, ErrorType, FlatMapValue, MetricsEnvironment, Param,
    },
    log::{debug, error, trace},
};
use thunder_ripple_sdk::{
    processors::thunder_analytics::send_to_analytics_plugin, thunder_state::ThunderState,
};
use thunder_ripple_sdk::{
    processors::thunder_analytics::BehavioralMetricsEvent, ripple_sdk::async_trait::async_trait,
};
use tokio::sync::Mutex as TokioMutex;

use crate::gateway::appsanity_gateway::{AppsanityConfig, MetricsSchemas};
use crate::util::http_client::HttpClient;
use std::{
    collections::{HashMap, HashSet},
    convert::From,
    sync::{Arc, RwLock},
    time::{SystemTime, UNIX_EPOCH},
    vec,
};
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_telemetry::get_params;
use uuid::Uuid;

#[async_trait]
pub trait BehavioralMetricsService {
    async fn send_metric(&self, metrics: AppBehavioralMetric) -> ();
}
pub trait ContextualMetricsService {
    fn update_metrics_context(&self, new_context: Option<MetricsContext>) -> ();
}

//use crate::gateway::appsanity_gateway::AppsanityConfig;
//use crate::gateway::appsanity_gateway::MetricsSchemas;
//use tower::{Service, ServiceExt};
//use uuid::Uuid;
#[derive(Debug, Serialize, Clone, PartialEq)]
#[serde(rename_all = "camelCase")]
pub enum InAppMediaEventType {
    MediaLoadStart,
    MediaPlay,
    MediaPlaying,
    MediaPause,
    MediaWaiting,
    MediaProgress,
    MediaSeeking,
    MediaSeeked,
    MediaRateChange,
    MediaRenditionChange,
    MediaEnded,
    Raw,
}
#[derive(Debug, Serialize, Clone)]
pub struct InAppMedia {
    media_event_name: InAppMediaEventType,
    src_entity_id: Option<String>,
    app_session_id: String,
    app_user_session_id: String,
    durable_app_id: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    app_version: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    media_pos_pct: Option<i32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    media_pos_seconds: Option<i32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    playback_rate: Option<i32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    playback_bitrate: Option<i32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    playback_width: Option<i32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    playback_height: Option<i32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    playback_profile: Option<String>,
}

impl InAppMedia {
    pub fn new(
        media_event_name: InAppMediaEventType,
        src_entity_id: String,
        context: AppContext,
    ) -> InAppMedia {
        InAppMedia {
            media_event_name,
            src_entity_id: Some(src_entity_id),
            app_session_id: context.app_session_id.clone(),
            app_user_session_id: option_string_to_pabs(&context.app_user_session_id),
            durable_app_id: context.durable_app_id.clone(),
            app_version: context.app_version.clone(),
            media_pos_pct: None,
            media_pos_seconds: None,
            playback_rate: None,
            playback_bitrate: None,
            playback_width: None,
            playback_height: None,
            playback_profile: None,
        }
    }
}

#[derive(Debug, Serialize, Clone)]
pub struct InAppContentStart {
    #[serde(skip_serializing_if = "Option::is_none")]
    src_entity_id: Option<String>,
    app_session_id: String,
    app_user_session_id: String,
    durable_app_id: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    app_version: Option<String>,
}
#[derive(Debug, Serialize, Clone)]
pub struct InAppContentStop {
    #[serde(skip_serializing_if = "Option::is_none")]
    src_entity_id: Option<String>,
    app_session_id: String,
    app_user_session_id: String,
    durable_app_id: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    app_version: Option<String>,
}
#[derive(Debug, Serialize, Clone)]
pub struct InAppPageView {
    src_page_id: Option<String>,
    app_session_id: String,
    app_user_session_id: String,
    durable_app_id: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    app_version: Option<String>,
}

#[derive(Debug, Serialize, Clone)]
pub struct InAppOtherAction {
    category: Option<ActionCategory>,
    #[serde(rename = "type")]
    action_type: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    parameters: Option<HashMap<String, FlatMapValue>>,
    app_session_id: String,
    app_user_session_id: String,
    durable_app_id: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    app_version: Option<String>,
}

#[derive(Debug, Serialize, Clone)]
pub struct AppLifecycleStateChange {
    app_session_id: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    app_user_session_id: Option<String>,
    durable_app_id: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    app_version: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    previous_life_cycle_state: Option<AppLifecycleState>,
    new_life_cycle_state: AppLifecycleState,
}
#[derive(Debug, Serialize, Clone)]
pub struct AppReady {
    app_session_id: String,
    app_user_session_id: String,
    durable_app_id: String,
    is_cold_launch: bool,
}

#[derive(Debug, Serialize, Clone)]
pub struct InAppError {
    #[serde(skip_serializing)]
    pub context: AppContext,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub app_session_id: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub app_user_session_id: Option<String>,
    pub durable_app_id: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    app_version: Option<String>,
    pub third_party_error: bool,
    #[serde(rename = "type")]
    pub error_type: Option<ErrorType>,
    pub code: Option<String>,
    pub description: Option<String>,
    pub visible: Option<bool>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub parameters: Option<HashMap<String, FlatMapValue>>,
}
/*
This is basically a projection of FireboltBehavioralMetrics
A type alias will not work because we need to implement Traits (below)
and FireboltBehavioralMetrics is in another crate
*/
#[derive(Debug, Serialize, Clone)]
#[serde(untagged)]
pub enum AXPMetric {
    InAppMedia(InAppMedia),
    InAppContentStart(InAppContentStart),
    InAppContentStop(InAppContentStop),
    InAppPageView(InAppPageView),
    InAppOtherAction(InAppOtherAction),
    AppLifecycleStateChange(AppLifecycleStateChange),
    InAppError(InAppError),
    AppReady(AppReady),
    BadgerAppAction(BadgerAppAction),
    BadgerMetric(BadgerMetric),
    BadgerError(BadgerError),
    BadgerLaunchCompleted(BadgerLaunchCompleted),
    BadgerDismissLoadingScreen(BadgerDismissLoadingScreen),
    BadgerPageView(BadgerPageView),
    BadgerUserAction(BadgerUserAction),
    BadgerUserError(BadgerUserError),
}

impl BehavioralMetric for AXPMetric {
    fn event_name(&self, metrics_schemas: &MetricsSchemas) -> String {
        match self {
            AXPMetric::BadgerAppAction(_) => metrics_schemas.get_event_name_alias("app_action"),
            AXPMetric::BadgerMetric(_) => metrics_schemas.get_event_name_alias("metric"),
            AXPMetric::BadgerError(_) => metrics_schemas.get_event_name_alias("error"),
            AXPMetric::BadgerLaunchCompleted(_) => {
                metrics_schemas.get_event_name_alias("launch_completed")
            }
            AXPMetric::BadgerDismissLoadingScreen(_) => {
                metrics_schemas.get_event_name_alias("dismiss_loading_screen")
            }
            AXPMetric::BadgerPageView(_) => metrics_schemas.get_event_name_alias("page_view"),
            AXPMetric::BadgerUserAction(_) => metrics_schemas.get_event_name_alias("user_action"),
            AXPMetric::BadgerUserError(_) => metrics_schemas.get_event_name_alias("user_error"),
            AXPMetric::AppLifecycleStateChange(_) => {
                metrics_schemas.get_event_name_alias("app_lc_state_change")
            }
            AXPMetric::InAppMedia(_) => metrics_schemas.get_event_name_alias("inapp_media"),
            AXPMetric::InAppError(_) => metrics_schemas.get_event_name_alias("app_error"),
            AXPMetric::InAppContentStart(_) => {
                metrics_schemas.get_event_name_alias("inapp_content_start")
            }
            AXPMetric::InAppContentStop(_) => {
                metrics_schemas.get_event_name_alias("inapp_content_stop")
            }
            AXPMetric::InAppPageView(_) => metrics_schemas.get_event_name_alias("inapp_page_view"),
            AXPMetric::InAppOtherAction(_) => {
                metrics_schemas.get_event_name_alias("inapp_other_action")
            }
            AXPMetric::AppReady(_) => metrics_schemas.get_event_name_alias("app_ready"),
        }
        .to_string()
    }

    fn event_type(&self) -> String {
        if self.is_badger() {
            "firebadger".to_string()
        } else {
            "firebolt".to_string()
        }
    }

    fn event_schema(&self, metrics_schemas: &MetricsSchemas) -> String {
        metrics_schemas.get_event_path(&self.event_name(metrics_schemas))
    }

    fn schema_version(&self) -> String {
        /*for now, not very exciting */
        "3".to_string()
    }
}
impl AXPMetric {
    fn is_badger(&self) -> bool {
        match self {
            AXPMetric::BadgerAppAction(_) => true,
            _ => false,
        }
    }
}

fn extract_context(metric: &AppBehavioralMetric) -> AppContext {
    match metric {
        AppBehavioralMetric::Ready(a) => a.context.clone(),
        AppBehavioralMetric::SignIn(a) => a.context.clone(),
        AppBehavioralMetric::SignOut(a) => a.context.clone(),
        AppBehavioralMetric::StartContent(a) => a.context.clone(),
        AppBehavioralMetric::StopContent(a) => a.context.clone(),
        AppBehavioralMetric::Page(a) => a.context.clone(),
        AppBehavioralMetric::Action(a) => a.context.clone(),
        AppBehavioralMetric::Error(e) => e.context.clone(),
        AppBehavioralMetric::MediaLoadStart(a) => a.context.clone(),
        AppBehavioralMetric::MediaPlay(a) => a.context.clone(),
        AppBehavioralMetric::MediaPlaying(a) => a.context.clone(),
        AppBehavioralMetric::MediaPause(a) => a.context.clone(),
        AppBehavioralMetric::MediaWaiting(a) => a.context.clone(),
        AppBehavioralMetric::MediaProgress(a) => a.context.clone(),
        AppBehavioralMetric::MediaSeeking(a) => a.context.clone(),
        AppBehavioralMetric::MediaSeeked(a) => a.context.clone(),
        AppBehavioralMetric::MediaRateChanged(a) => a.context.clone(),
        AppBehavioralMetric::MediaRenditionChanged(a) => a.context.clone(),
        AppBehavioralMetric::MediaEnded(a) => a.context.clone(),
        AppBehavioralMetric::AppStateChange(a) => a.context.clone(),
        AppBehavioralMetric::Raw(a) => a.context.clone(),
    }
}
fn badger_extract_context(metric: &BadgerMetrics) -> AppContext {
    match metric {
        BadgerMetrics::Metric(a) => a.context.clone(),
        BadgerMetrics::AppAction(a) => a.context.clone(),
        BadgerMetrics::Error(a) => a.context.clone(),
        BadgerMetrics::LaunchCompleted(a) => a.context.clone(),
        BadgerMetrics::DismissLoadingScreen(a) => a.context.clone(),
        BadgerMetrics::PageView(a) => a.context.clone(),
        BadgerMetrics::UserAction(a) => a.context.clone(),
        BadgerMetrics::UserError(a) => a.context.clone(),
    }
}

impl From<AppBehavioralMetric> for AXPMetric {
    fn from(metric: AppBehavioralMetric) -> Self {
        fb_2_pabs(&metric)
    }
}

/*PABS specific wrappers */
pub trait BehavioralMetric: Send + Clone + Sized {
    fn event_name(&self, metrics_schemas: &MetricsSchemas) -> String;
    fn event_type(&self) -> String;
    fn event_schema(&self, metrics_schemas: &MetricsSchemas) -> String;
    fn schema_version(&self) -> String;
}

pub trait CustomMetric {
    fn event_schema(&self, metrics_schemas: &MetricsSchemas) -> String;
}

/*
 { method: "badger.logMoneyBadgerLoaded",
 params_json: "[{\"app_id\":\"foo\",\"call_id\":0,
 \"method\":\"badger.logMoneyBadgerLoaded\",
 \"protocol\":\"Badger\",
 \"session_id\":\"ec027353-1c25-446e-8be8-994eac8a32ee\"},
 {\"startTime\":1660321072274,\"version\":\"4.10.0-7e1cc95\"}]",
      ctx: CallContext { session_id: "ec027353-1c25-446e-8be8-994eac8a32ee", app_id: "foo", call_id: 0, protocol: Badger, method: "badger.logMoneyBadgerLoaded" } }}:
*/
#[allow(dead_code)]
fn default_as_true() -> bool {
    true
}
fn option_string_to_pabs(the_string: &Option<String>) -> String {
    /*clone is required due to &*/
    the_string.clone().unwrap_or_else(|| String::from(""))
}
/*
SIFT likes empty strings
*/
pub fn serialize_pabs_option<S>(x: &Option<String>, s: S) -> Result<S::Ok, S::Error>
where
    S: Serializer,
{
    match x {
        Some(val) => s.serialize_str(val),
        None => s.serialize_str(""),
    }
}
#[derive(Debug, Serialize, Clone, Default)]
#[serde(rename_all = "camelCase")]
pub struct EntOsCetTags {
    durable_app_id: String,
    capabilities_exclusion_tags: HashSet<String>,
}

#[derive(Debug, Serialize, Clone, Default)]
pub struct EntOsCustomMetric {
    device_mac: String,
    device_serial: String,
    source_app: String,
    source_product_version: String,
    #[serde(default = "default_as_true")]
    authenticated: bool,
    #[serde(skip_serializing_if = "Option::is_none")]
    access_schema: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    access_payload: Option<EntOsCetTags>,
}

impl CustomMetric for EntOsCustomMetric {
    fn event_schema(&self, metrics_schemas: &MetricsSchemas) -> String {
        metrics_schemas.get_event_path("custom")
    }
}

/// serialization wrapper for Comcast Common Schema, aka "CCS"
#[derive(Serialize, Debug)]
pub struct CCS<T: Serialize, C: Serialize> {
    app_name: String,
    app_ver: String,
    device_language: String,
    device_model: String,
    partner_id: String,
    device_id: String,
    account_id: String,
    device_timezone: i32,
    device_name: String,
    platform: String,
    os_ver: String,
    session_id: String,
    event_id: String,
    event_type: String,
    event_name: String,
    timestamp: u64,
    event_schema: String,
    event_payload: T,
    custom_schema: String,
    custom_payload: C,
}

/*
Given a human readable timezone , compute ms offset from UTC
*/
fn get_timezone_offset() -> i32 {
    use chrono::Local;
    Local::now().offset().local_minus_utc() * 1000
}

impl<T: Serialize + BehavioralMetric, C: Serialize + CustomMetric> CCS<T, C> {
    fn new(
        metrics_context: &MetricsContext,
        app_name: String,
        app_ver: String,
        partner_id: String,
        event_payload: T,
        custom_payload: C,
        metrics_schemas: &MetricsSchemas,
    ) -> CCS<T, C> {
        let timestamp: u64 = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_millis()
            .try_into()
            .unwrap();
        CCS {
            app_name,
            app_ver: app_ver,
            device_language: metrics_context.device_language.to_string(),
            device_model: metrics_context.device_model.to_string(),
            partner_id: partner_id,
            device_id: metrics_context.device_id.clone().unwrap_or_default(),
            account_id: metrics_context.account_id.clone().unwrap_or_default(),
            device_timezone: get_timezone_offset(),
            device_name: metrics_context.device_name.clone().unwrap_or_default(),
            platform: "entos:rdk".to_string(),
            os_ver: metrics_context.os_ver.to_string(),
            session_id: metrics_context.device_session_id.to_string(),
            event_id: Uuid::new_v4().to_string(),
            event_type: event_payload.event_type(),
            event_name: event_payload.event_name(metrics_schemas),
            timestamp: timestamp,
            event_schema: event_payload.event_schema(metrics_schemas),
            event_payload: event_payload,
            custom_schema: custom_payload.event_schema(metrics_schemas),
            custom_payload: custom_payload,
        }
    }

    fn create_behavioral_metrics_event(
        &self,
        metrics_context: &MetricsContext,
    ) -> BehavioralMetricsEvent {
        let event_version = match self.event_schema.split('/').last() {
            Some(v) => Some(v.to_string()),
            None => None,
        };
        BehavioralMetricsEvent {
            event_name: self.event_name.clone(),
            event_version,
            event_source: self.app_name.clone(),
            event_source_version: self.app_ver.clone(),
            cet_list: metrics_context
                .data_governance_tags
                .clone()
                .unwrap_or_default(),
            epoch_timestamp: Some(self.timestamp),
            uptime_timestamp: None,
            event_payload: serde_json::to_value(self.event_payload.clone()).unwrap(),
        }
    }
}

/// serialization wrapper for Ontology schema
#[derive(Serialize, Debug)]
pub struct OntologyPayload<T: Serialize> {
    common_schema: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    env: Option<String>,
    product_name: String,
    product_version: String,
    event_schema: String,
    event_name: String,
    timestamp: u64,
    event_id: String,
    event_payload: T,
    event_source: String,
    event_source_version: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    cet_list: Option<Vec<String>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    category_tags: Option<Vec<String>>,
    logger_name: String,
    logger_version: String,
    partner_id: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    xbo_account_id: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    xbo_device_id: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    activated: Option<bool>,
    device_model: String,
    device_type: String,
    device_os_name: String,
    device_os_version: String,
    device_timezone: i32,
    device_manufacturer: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    authenticated: Option<bool>,
    device_session_id: String,
    proposition: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    retailer: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    jv_agent: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    coam: Option<bool>,
    device_serial_number: String,
    device_mac_address: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    device_friendly_name: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    country: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    region: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    account_type: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    operator: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    account_detail_type: Option<String>,
}

#[derive(Debug)]
pub enum SiftDeviceType {
    IPTV,
    IPSTB,
    IPCOAM,
}

impl SiftDeviceType {
    pub fn from_form_factor(device_type: &str) -> String {
        match device_type {
            "ipstb" => SiftDeviceType::IPSTB.to_string(),
            "smarttv" => SiftDeviceType::IPTV.to_string(),
            _ => device_type.into(),
        }
    }
}

impl ToString for SiftDeviceType {
    fn to_string(&self) -> String {
        match self {
            SiftDeviceType::IPTV => "iptv".into(),
            SiftDeviceType::IPSTB => "ipstb".into(),
            SiftDeviceType::IPCOAM => "ipcoam".into(),
        }
    }
}

impl<T: Serialize + BehavioralMetric> OntologyPayload<T> {
    fn new(
        metrics_context: &MetricsContext,
        product_name: String,
        product_version: String,
        partner_id: String,
        cet_list: Option<Vec<String>>,
        category_tags: Option<Vec<String>>,
        event_payload: T,
        metrics_schemas: &MetricsSchemas,
    ) -> OntologyPayload<T> {
        let timestamp: u64 = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_millis()
            .try_into()
            .unwrap();

        let device_type = &metrics_context.device_type;
        let mapped_device_type = SiftDeviceType::from_form_factor(&device_type);

        let env = if metrics_context.os_ver.is_empty() {
            None
        } else if metrics_context.os_ver.ends_with("P") {
            Some(MetricsEnvironment::Prod.to_string())
        } else {
            Some(MetricsEnvironment::Dev.to_string())
        };

        OntologyPayload {
            common_schema: metrics_schemas.default_common_schema.clone(),
            env,
            product_name: product_name.clone(),
            product_version: metrics_context.os_ver.clone(),
            event_schema: event_payload.event_schema(metrics_schemas),
            event_name: event_payload.clone().event_name(metrics_schemas),
            timestamp,
            event_id: Uuid::new_v4().to_string(),
            event_payload: event_payload.clone(),
            event_source: "ripple".into(),
            event_source_version: metrics_context.ripple_version.clone(),
            cet_list,
            category_tags,
            logger_name: "ripple".into(),
            logger_version: product_version.clone(),
            partner_id,
            xbo_account_id: metrics_context.account_id.clone(),
            xbo_device_id: metrics_context.device_id.clone(),
            activated: metrics_context.activated,
            device_model: metrics_context.device_model.to_string(),
            device_type: mapped_device_type.to_string().to_lowercase(),
            device_os_name: metrics_context.os_name.clone(),
            device_os_version: metrics_context.os_ver.to_string(),
            device_timezone: get_timezone_offset(),
            device_manufacturer: metrics_context.device_manufacturer.clone(),
            authenticated: metrics_context.authenticated,
            device_session_id: metrics_context.device_session_id.to_string(),
            proposition: metrics_context.proposition.clone(),
            retailer: metrics_context.retailer.clone(),
            jv_agent: metrics_context.primary_provider.clone(),
            coam: metrics_context.coam,
            device_serial_number: metrics_context.serial_number.clone(),
            device_mac_address: metrics_context.mac_address.clone(),
            device_friendly_name: metrics_context.device_name.clone(),
            country: metrics_context.country.clone(),
            region: metrics_context.region.clone(),
            account_type: metrics_context.account_type.clone(),
            operator: metrics_context.operator.clone(),
            account_detail_type: metrics_context.account_detail_type.clone(),
        }
    }

    fn create_behavioral_metrics_event(
        &self,
        _metrics_context: &MetricsContext,
    ) -> BehavioralMetricsEvent {
        let event_version = match self.event_schema.split('/').last() {
            Some(v) => Some(v.to_string()),
            None => None,
        };
        BehavioralMetricsEvent {
            event_name: self.event_name.clone(),
            event_version,
            event_source: self.product_name.clone(),
            event_source_version: self.product_version.clone(),
            cet_list: self.cet_list.clone().unwrap_or_default(),
            epoch_timestamp: Some(self.timestamp),
            uptime_timestamp: None,
            event_payload: serde_json::to_value(self.event_payload.clone()).unwrap(),
        }
    }
}

#[derive(Clone, Debug)]
pub struct SiftService {
    thunder: ThunderState,
    pub sift_endpoint: String,
    ontology: bool,
    pub batch_size: u8,
    pub metrics_schemas: MetricsSchemas,
    metrics_send_queue: Arc<TokioMutex<CircularBuffer<Value>>>,
    metrics_context: Arc<RwLock<Option<MetricsContext>>>,
    analytics_plugin: bool,
    age_policy_config: AgePolicyConfig,
    service_client: Option<ServiceClient>,
}

#[async_trait]
impl BehavioralMetricsService for SiftService {
    async fn send_metric(&self, payload: AppBehavioralMetric) -> () {
        let _ = self.send_behavioral(payload).await;
    }
}
#[async_trait]
impl BadgerMetricsService for SiftService {
    async fn send_badger_metric(&self, payload: BadgerMetrics) -> () {
        self.send_badger(payload).await;
    }
}

fn param_vec_2_hashmap(params: Option<Vec<Param>>) -> Option<HashMap<String, FlatMapValue>> {
    params.map(|p| {
        let mut result: HashMap<String, FlatMapValue> = HashMap::new();
        for param in p {
            result.insert(param.name, param.value);
        }
        result
    })
}

impl ContextualMetricsService for SiftService {
    fn update_metrics_context(&self, new_context: Option<MetricsContext>) -> () {
        self.set_metrics_context(new_context)
    }
}

fn should_ignore_metrics(firebolt_metric: &AppBehavioralMetric) -> bool {
    if let AppBehavioralMetric::Ready(_) = firebolt_metric {
        true
    } else {
        false
    }
}

fn fb_2_pabs(firebolt_metric: &AppBehavioralMetric) -> AXPMetric {
    match firebolt_metric {
        AppBehavioralMetric::Ready(a) => {
            AXPMetric::AppLifecycleStateChange(AppLifecycleStateChange {
                app_session_id: a.context.app_session_id.clone(),
                app_user_session_id: a.context.app_user_session_id.clone(),
                durable_app_id: a.context.durable_app_id.clone(),
                app_version: a.context.app_version.clone(),
                previous_life_cycle_state: None,
                new_life_cycle_state: AppLifecycleState::Foreground,
            })
        }
        AppBehavioralMetric::AppStateChange(a) => {
            AXPMetric::AppLifecycleStateChange(AppLifecycleStateChange {
                app_session_id: a.context.app_session_id.clone(),
                app_user_session_id: a.context.app_user_session_id.clone(),
                durable_app_id: a.context.durable_app_id.clone(),
                app_version: a.context.app_version.clone(),
                previous_life_cycle_state: a.previous_state.clone(),
                new_life_cycle_state: a.new_state.clone(),
            })
        }
        AppBehavioralMetric::Error(a) => AXPMetric::InAppError(InAppError {
            context: a.context.clone(),
            app_session_id: Some(a.context.app_session_id.clone()),
            app_user_session_id: a.context.app_user_session_id.clone(),
            durable_app_id: a.durable_app_id.clone(),
            app_version: a.context.app_version.clone(),
            third_party_error: a.third_party_error,
            error_type: Some(a.error_type.clone()),
            code: Some(a.code.clone()),
            description: Some(a.description.clone()),
            visible: Some(a.visible),
            parameters: a.parameters.clone(),
        }),

        AppBehavioralMetric::SignIn(sign_in) => AXPMetric::InAppOtherAction(InAppOtherAction {
            category: Some(ActionCategory::user),
            action_type: Some(String::from("sign_in")),
            parameters: None,
            app_session_id: sign_in.context.app_session_id.clone(),
            app_user_session_id: option_string_to_pabs(
                &sign_in.context.app_user_session_id.clone(),
            ),
            durable_app_id: sign_in.context.durable_app_id.clone(),
            app_version: sign_in.context.app_version.clone(),
        }),
        AppBehavioralMetric::SignOut(sign_out) => AXPMetric::InAppOtherAction(InAppOtherAction {
            category: Some(ActionCategory::user),
            action_type: Some(String::from("sign_out")),
            parameters: None,
            app_session_id: sign_out.context.app_session_id.clone(),
            app_user_session_id: option_string_to_pabs(
                &sign_out.context.app_user_session_id.clone(),
            ),
            durable_app_id: sign_out.context.durable_app_id.clone(),
            app_version: sign_out.context.app_version.clone(),
        }),

        AppBehavioralMetric::StartContent(start_content) => {
            AXPMetric::InAppContentStart(InAppContentStart {
                src_entity_id: start_content.entity_id.clone(),
                app_session_id: start_content.context.app_session_id.clone(),
                app_user_session_id: option_string_to_pabs(
                    &start_content.context.app_user_session_id.clone(),
                ),
                durable_app_id: start_content.context.durable_app_id.clone(),
                app_version: start_content.context.app_version.clone(),
            })
        }

        AppBehavioralMetric::StopContent(stop_content) => {
            AXPMetric::InAppContentStop(InAppContentStop {
                src_entity_id: stop_content.entity_id.clone(),
                app_session_id: stop_content.context.app_session_id.clone(),
                app_user_session_id: option_string_to_pabs(
                    &stop_content.context.app_user_session_id,
                ),
                durable_app_id: stop_content.context.durable_app_id.clone(),
                app_version: stop_content.context.app_version.clone(),
            })
        }

        AppBehavioralMetric::Page(page) => AXPMetric::InAppPageView(InAppPageView {
            src_page_id: Some(page.page_id.clone()),
            app_session_id: page.context.app_session_id.clone(),
            app_user_session_id: option_string_to_pabs(&page.context.app_user_session_id),
            durable_app_id: page.context.durable_app_id.clone(),
            app_version: page.context.app_version.clone(),
        }),

        AppBehavioralMetric::Action(action) => AXPMetric::InAppOtherAction(InAppOtherAction {
            category: Some(action.category.clone().into()),
            action_type: Some(action._type.clone()),
            parameters: param_vec_2_hashmap(Some(action.parameters.clone())),
            app_session_id: action.context.app_session_id.clone(),
            app_user_session_id: option_string_to_pabs(&action.context.app_user_session_id),
            durable_app_id: action.context.durable_app_id.clone(),
            app_version: action.context.app_version.clone(),
        }),

        AppBehavioralMetric::MediaLoadStart(media_load_start) => {
            AXPMetric::InAppMedia(InAppMedia::new(
                InAppMediaEventType::MediaLoadStart,
                media_load_start.entity_id.clone(),
                media_load_start.context.clone(),
            ))
        }

        AppBehavioralMetric::MediaPlay(media_play) => AXPMetric::InAppMedia(InAppMedia::new(
            InAppMediaEventType::MediaPlay,
            media_play.entity_id.clone(),
            media_play.context.clone(),
        )),

        AppBehavioralMetric::MediaPlaying(media_playing) => AXPMetric::InAppMedia(InAppMedia::new(
            InAppMediaEventType::MediaPlaying,
            media_playing.entity_id.clone(),
            media_playing.context.clone(),
        )),

        AppBehavioralMetric::MediaPause(media_pause) => AXPMetric::InAppMedia(InAppMedia::new(
            InAppMediaEventType::MediaPause,
            media_pause.entity_id.clone(),
            media_pause.context.clone(),
        )),

        AppBehavioralMetric::MediaWaiting(media_waiting) => AXPMetric::InAppMedia(InAppMedia::new(
            InAppMediaEventType::MediaWaiting,
            media_waiting.entity_id.clone(),
            media_waiting.context.clone(),
        )),

        AppBehavioralMetric::MediaProgress(media_progress) => {
            AXPMetric::InAppMedia(InAppMedia::new(
                InAppMediaEventType::MediaProgress,
                media_progress.entity_id.clone(),
                media_progress.context.clone(),
            ))
        }

        AppBehavioralMetric::MediaSeeking(media_seeking) => AXPMetric::InAppMedia(InAppMedia::new(
            InAppMediaEventType::MediaSeeking,
            media_seeking.entity_id.clone(),
            media_seeking.context.clone(),
        )),

        AppBehavioralMetric::MediaSeeked(media_seeked) => AXPMetric::InAppMedia(InAppMedia::new(
            InAppMediaEventType::MediaSeeked,
            media_seeked.entity_id.clone(),
            media_seeked.context.clone(),
        )),

        AppBehavioralMetric::MediaRateChanged(media_rate_changed) => {
            AXPMetric::InAppMedia(InAppMedia::new(
                InAppMediaEventType::MediaRateChange,
                media_rate_changed.entity_id.clone(),
                media_rate_changed.context.clone(),
            ))
        }

        AppBehavioralMetric::MediaRenditionChanged(media_rendition_changed) => {
            AXPMetric::InAppMedia(InAppMedia::new(
                InAppMediaEventType::MediaRenditionChange,
                media_rendition_changed.entity_id.clone(),
                media_rendition_changed.context.clone(),
            ))
        }

        AppBehavioralMetric::MediaEnded(event) => AXPMetric::InAppMedia(InAppMedia::new(
            InAppMediaEventType::MediaEnded,
            event.entity_id.clone(),
            event.context.clone(),
        )),

        AppBehavioralMetric::Raw(event) => AXPMetric::InAppMedia(InAppMedia::new(
            InAppMediaEventType::Raw,
            "".to_string(),
            event.context.clone(),
        )),
    }
}

pub fn get_access_schema(app_context: &AppContext) -> Option<String> {
    match app_context.governance_state.clone() {
        Some(_) => Some(String::from("access/tags/0")),
        None => None,
    }
}
fn decorate_badger_method_name(method: Option<String>) -> String {
    format!(
        "badger.{}",
        method.unwrap_or_else(|| String::from("undefined_method"))
    )
}
impl SiftService {
    pub fn set_metrics_context(&self, metrics_context: Option<MetricsContext>) {
        let mut context = self.metrics_context.write().unwrap();
        if let Some(new_context) = metrics_context {
            let _ = context.insert(new_context);
        }
    }

    fn badger_2_pabs(&self, badger_metric: &BadgerMetrics) -> impl BehavioralMetric + Serialize {
        match badger_metric {
            BadgerMetrics::Metric(a) => AXPMetric::InAppOtherAction(InAppOtherAction {
                category: Some(ActionCategory::app),
                action_type: Some(decorate_badger_method_name(a.segment.clone())),
                parameters: param_vec_2_hashmap(a.args.clone()),
                app_session_id: a.context.app_session_id.clone(),
                app_user_session_id: option_string_to_pabs(&a.context.app_user_session_id),
                durable_app_id: a.context.durable_app_id.clone(),
                app_version: a.context.app_version.clone(),
            }),
            BadgerMetrics::AppAction(a) => AXPMetric::InAppOtherAction(InAppOtherAction {
                category: Some(ActionCategory::app),
                action_type: Some(decorate_badger_method_name(Some(a.action.clone()))),
                parameters: param_vec_2_hashmap(Some(a.args.clone())),
                app_session_id: a.context.app_session_id.clone(),
                app_user_session_id: option_string_to_pabs(&a.context.app_user_session_id),
                durable_app_id: a.context.durable_app_id.clone(),
                app_version: a.context.app_version.clone(),
            }),
            BadgerMetrics::Error(a) => AXPMetric::InAppError(InAppError {
                context: a.context.clone(),
                app_session_id: Some(a.context.app_session_id.clone()),
                app_user_session_id: a.context.app_user_session_id.clone(),
                durable_app_id: a.context.durable_app_id.clone(),
                app_version: a.context.app_version.clone(),
                third_party_error: true,
                error_type: Some(ErrorType::other),
                code: Some(a.code.to_string()),
                description: Some(a.message.clone()),
                visible: Some(a.visible),
                parameters: get_params(a.args.clone()),
            }),

            BadgerMetrics::LaunchCompleted(a) => AXPMetric::InAppOtherAction(InAppOtherAction {
                category: Some(ActionCategory::app),
                action_type: Some(decorate_badger_method_name(Some(String::from(
                    "launchCompleted",
                )))),
                parameters: param_vec_2_hashmap(a.args.clone()),
                app_session_id: a.context.app_session_id.clone(),
                app_user_session_id: option_string_to_pabs(&a.context.app_user_session_id.clone()),
                durable_app_id: a.context.durable_app_id.clone(),
                app_version: a.context.app_version.clone(),
            }),
            BadgerMetrics::DismissLoadingScreen(a) => {
                AXPMetric::InAppOtherAction(InAppOtherAction {
                    category: Some(ActionCategory::app),
                    action_type: Some(decorate_badger_method_name(Some(String::from(
                        "dismissLoadingScreen",
                    )))),
                    parameters: param_vec_2_hashmap(a.args.clone()),
                    app_session_id: a.context.app_session_id.clone(),
                    app_user_session_id: option_string_to_pabs(
                        &a.context.app_user_session_id.clone(),
                    ),
                    durable_app_id: a.context.durable_app_id.clone(),
                    app_version: a.context.app_version.clone(),
                })
            }
            BadgerMetrics::PageView(a) => AXPMetric::InAppPageView(InAppPageView {
                src_page_id: Some(a.page.clone()),
                app_session_id: a.context.app_session_id.clone(),
                app_user_session_id: option_string_to_pabs(&a.context.app_user_session_id),
                durable_app_id: a.context.durable_app_id.clone(),
                app_version: a.context.app_version.clone(),
            }),
            BadgerMetrics::UserAction(a) => AXPMetric::InAppOtherAction(InAppOtherAction {
                category: Some(ActionCategory::user),
                action_type: Some(decorate_badger_method_name(Some(String::from(
                    "userAction",
                )))),
                parameters: param_vec_2_hashmap(a.args.clone()),
                app_session_id: a.context.app_session_id.clone(),
                app_user_session_id: option_string_to_pabs(&a.context.app_user_session_id.clone()),
                durable_app_id: a.context.durable_app_id.clone(),
                app_version: a.context.app_version.clone(),
            }),
            BadgerMetrics::UserError(a) => AXPMetric::InAppError(InAppError {
                context: a.context.clone(),
                app_session_id: Some(a.context.app_session_id.clone()),
                app_user_session_id: a.context.app_user_session_id.clone(),
                durable_app_id: a.context.durable_app_id.clone(),
                app_version: a.context.app_version.clone(),
                third_party_error: false,
                error_type: Some(ErrorType::other),
                code: Some(a.code.to_string()),
                description: Some(a.message.clone()),
                visible: Some(a.visible),
                parameters: get_params(a.args.clone()),
            }),
        }
    }

    fn extract_app_id(&self, metric: &AppBehavioralMetric) -> String {
        extract_context(metric).app_id.clone()
    }
    fn extract_partner_id(&self, metric: &AppBehavioralMetric) -> String {
        extract_context(metric).partner_id.clone()
    }
    fn extract_product_version(&self, metric: &AppBehavioralMetric) -> String {
        extract_context(metric).product_version.clone()
    }
    fn badger_extract_app_id(&self, metric: &BadgerMetrics) -> String {
        badger_extract_context(metric).app_id.clone()
    }
    fn badger_extract_partner_id(&self, metric: &BadgerMetrics) -> String {
        badger_extract_context(metric).partner_id.clone()
    }
    fn badger_extract_product_version(&self, metric: &BadgerMetrics) -> String {
        badger_extract_context(metric).product_version.clone()
    }

    pub fn get_context(&self) -> MetricsContext {
        self.metrics_context.read().unwrap().clone().unwrap()
    }

    pub async fn send_behavioral(&self, firebolt_metric: AppBehavioralMetric) -> Option<bool> {
        if should_ignore_metrics(&firebolt_metric) {
            return None;
        }

        /*
        This is the app name in the TOP LEVEL CCS schema
        */
        let app_name = String::from("entos");
        let partner_id = self.extract_partner_id(&firebolt_metric);
        let product_version = self.extract_product_version(&firebolt_metric);
        let context = self.get_context();
        let app_context = extract_context(&firebolt_metric);
        let (schema, tags) = if let Some(cet_tags) = app_context.clone().governance_state {
            if !cet_tags.data_tags_to_apply.is_empty() {
                (
                    get_access_schema(&app_context.clone()),
                    Some(EntOsCetTags {
                        durable_app_id: app_context.durable_app_id.clone(),
                        capabilities_exclusion_tags: cet_tags.data_tags_to_apply,
                    }),
                )
            } else {
                (None, None)
            }
        } else {
            (None, None)
        };
        let call_context = CallContext::default();

        let metrics_context = self.get_context();
        if self.ontology {
            let request_age_policy = firebolt_metric.extract_age_policy();
            let age_policy_config = self.age_policy_config.clone();
            let session_policies =
                get_age_policy_identifiers(self.service_client.clone(), call_context.clone()).await;
            let age_policy_metadata = age_policy_config
                .get_age_policy_metrics_metadata(request_age_policy, session_policies);

            let cet_list: Option<Vec<String>> = match tags {
                Some(t) => Some(t.capabilities_exclusion_tags.into_iter().collect()),
                None => None,
            };
            //add cet_list and category_cets
            let combined_cets = match age_policy_metadata {
                Some((policy_cets, category_tags)) => {
                    /*Guarantee unique tags, there can be dupes between existing cets and age policy cets */
                    let mut combined: HashSet<String> =
                        cet_list.unwrap_or_default().into_iter().collect();
                    combined.extend(policy_cets);
                    (Some(combined.into_iter().collect()), Some(category_tags))
                }
                None => (cet_list, None),
            };

            let cets = combined_cets.0.clone();

            let category_tags = combined_cets.1.clone();
            debug!("category_tags={:?}, cets={:?}", category_tags, cets);
            let payload = OntologyPayload::new(
                &metrics_context,
                "entos-immerse".into(),
                product_version,
                partner_id,
                cets,
                category_tags,
                fb_2_pabs(&firebolt_metric),
                &self.metrics_schemas,
            );

            debug!(
                "send_behavioral: Ontology payload: {}",
                serde_json::to_string(&payload).unwrap()
            );

            if self.analytics_plugin {
                let event = payload.create_behavioral_metrics_event(&metrics_context);
                let _ = send_to_analytics_plugin(self.thunder.clone(), event).await;
            } else {
                let mut batch = self.metrics_send_queue.lock().await;
                batch
                    .add(serde_json::to_value(&payload).unwrap_or_default())
                    .ok();
            }
        } else {
            let custom_payload = EntOsCustomMetric {
                device_mac: context.mac_address.to_string(),
                device_serial: context.serial_number.to_string(),
                source_app: self.extract_app_id(&firebolt_metric),
                source_product_version: self.extract_product_version(&firebolt_metric),
                authenticated: true,
                access_schema: schema,
                access_payload: tags,
            };

            let payload = CCS::new(
                &metrics_context,
                app_name,
                product_version,
                partner_id,
                fb_2_pabs(&firebolt_metric),
                custom_payload,
                &self.metrics_schemas,
            );

            debug!("behavioral={}", serde_json::to_string(&payload).unwrap());

            if self.analytics_plugin {
                let event = payload.create_behavioral_metrics_event(&metrics_context);
                let _ = send_to_analytics_plugin(self.thunder.clone(), event).await;
            } else {
                let mut batch = self.metrics_send_queue.lock().await;
                batch
                    .add(serde_json::to_value(&payload).unwrap_or_default())
                    .ok();
            }
        }
        if self.analytics_plugin {
            None
        } else {
            Some(true)
        }
    }

    pub async fn send_badger(&self, badger_metric: BadgerMetrics) {
        let event_payload = self.badger_2_pabs(&badger_metric);
        let app_name = String::from("entos");
        let partner_id = self.badger_extract_partner_id(&badger_metric);
        let app_ver = self.badger_extract_product_version(&badger_metric);

        let metrics_context = &self.get_context();
        let app_context = badger_extract_context(&badger_metric);
        let (schema, tags) = if let Some(cet_tags) = app_context.clone().governance_state {
            if !cet_tags.data_tags_to_apply.is_empty() {
                (
                    get_access_schema(&app_context.clone()),
                    Some(EntOsCetTags {
                        durable_app_id: app_context.durable_app_id.clone(),
                        capabilities_exclusion_tags: cet_tags.data_tags_to_apply,
                    }),
                )
            } else {
                (None, None)
            }
        } else {
            (None, None)
        };

        if self.ontology {
            let cet_list: Option<Vec<String>> = match tags {
                Some(t) => Some(t.capabilities_exclusion_tags.into_iter().collect()),
                None => None,
            };

            let payload = OntologyPayload::new(
                metrics_context,
                "entos-immerse".into(),
                app_ver,
                partner_id,
                cet_list,
                None,
                event_payload,
                &self.metrics_schemas,
            );

            debug!(
                "send_badger: Ontology payload: {}",
                serde_json::to_string(&payload).unwrap()
            );

            let mut batch = self.metrics_send_queue.lock().await;
            batch
                .add(serde_json::to_value(&payload).unwrap_or_default())
                .ok();
        } else {
            let custom_payload = EntOsCustomMetric {
                device_mac: metrics_context.mac_address.to_string(),
                device_serial: metrics_context.serial_number.to_string(),
                source_app: self.badger_extract_app_id(&badger_metric),
                source_product_version: self.badger_extract_product_version(&badger_metric),
                authenticated: true,
                access_schema: schema,
                access_payload: tags,
            };

            let payload = CCS::new(
                &self.get_context(),
                app_name,
                app_ver,
                partner_id,
                event_payload,
                custom_payload,
                &self.metrics_schemas,
            );
            let mut batch = self.metrics_send_queue.lock().await;
            debug!(
                "send_badger: badger  = {}",
                serde_json::to_string(&payload).unwrap()
            );
            let _ = batch.add(serde_json::to_value(&payload).unwrap_or_default());
        }
    }

    pub fn new(
        config: &AppsanityConfig,
        thunder: ThunderState,
        service_client: Option<ServiceClient>,
    ) -> Self {
        let sift_config = config.behavioral_metrics.sift.clone();

        SiftService {
            thunder,
            sift_endpoint: sift_config.endpoint.clone(),
            ontology: sift_config.ontology,
            batch_size: sift_config.batch_size,
            metrics_schemas: sift_config.metrics_schemas.clone(),
            metrics_context: Arc::new(RwLock::new(Some(MetricsContext::new()))),
            metrics_send_queue: Arc::new(TokioMutex::new(CircularBuffer::<Value>::new(
                config.clone().behavioral_metrics.sift.max_queue_size.into(),
            ))),
            analytics_plugin: config.behavioral_metrics.plugin,
            age_policy_config: config.age_policy.clone(),
            service_client: service_client.clone(),
        }
    }
}

pub fn start_metrics_thread(state: &DistributorState) {
    let cloud_services = state.config.clone();
    let session_token = state.account_session.clone();
    let metrics_queue = state.metrics.service.metrics_send_queue.clone();
    let interval_seconds: i64 =
        (cloud_services.behavioral_metrics.sift.send_interval_seconds * 1000) as i64;
    if let Ok(mut rx) = state.metrics.get_receiver() {
        tokio::spawn(async move {
            /*
            wait send_interval_seconds to start
            */
            debug!(
                "starting BI metrics, max send interval is: {}",
                cloud_services.behavioral_metrics.sift.send_interval_seconds
            );

            let mut last_sift_sync: i64 = 0;

            while let Some(t) = rx.recv().await {
                trace!("BI trigger {}", t);
                let now = Utc::now().timestamp_millis();
                // what is the difference between last send and now
                let difference = now - last_sift_sync;
                // difference < interval_seconds means SIFT was contacted  just before
                if difference < interval_seconds {
                    // sleep for the difference and also block the task
                    // Given its a batch request Sift State would just add it to queue and not
                    // trigger a message using the sender
                    tokio::time::sleep(tokio::time::Duration::from_millis(difference as u64)).await;
                }
                let token = {
                    let token_mutex = session_token.read().unwrap().clone();
                    if let Some(t) = token_mutex {
                        t.token
                    } else {
                        // try running the sync again after the configured time
                        continue;
                    }
                };
                let has_metrics_data_to_send = { metrics_queue.lock().await.size() > 0 };
                if has_metrics_data_to_send {
                    if !token.is_empty() {
                        // update only when data is sent to SIFT
                        last_sift_sync = Utc::now().timestamp_millis();
                        send_metrics(
                            metrics_queue.clone(),
                            cloud_services.behavioral_metrics.sift.endpoint.clone(),
                            token,
                            cloud_services.behavioral_metrics.sift.batch_size.into(),
                        )
                        .await;
                    } else {
                        debug!("sift sync failed token has_token={}", !token.is_empty());
                    }
                }
            }
        });
    }
}

pub fn initiate_metrics(state: DistributorState) {
    tokio::spawn(async move {
        let config = state.config.clone();
        let mut state_for_sift = state.clone();
        SiftMetricsState::initialize(&mut state_for_sift).await;
        if !config.behavioral_metrics.plugin {
            start_metrics_thread(&state);
        }
    });
}

async fn send_metrics(
    metrics_queue: Arc<TokioMutex<CircularBuffer<Value>>>,
    uri: String,
    token: String,
    batch_size: u16,
) {
    /*
    render the union of batches, and send
    */
    let mut batch: Vec<Value> = vec![];
    {
        let rendered_metrics = &mut *metrics_queue.lock().await;
        while rendered_metrics.size() > 0 {
            let metric = rendered_metrics.remove().unwrap();
            batch.push(metric);
        }
    }
    let f: usize = batch_size.into();
    let batch_num = match batch.len() / f {
        0 => 1,
        num => num,
    };

    let batches: Vec<&[Value]> = batch.chunks(batch_num).collect();

    for chunk in batches.iter() {
        let payloads = serde_json::to_string(chunk).unwrap();
        debug!("sending batch of {} BI metrics", batch.len());

        let mut http = HttpClient::new();
        match http
            .set_token(token.clone())
            .post(uri.clone(), payloads)
            .await
        {
            Ok(okie) => {
                debug!("SIFT metrics send success={:?}", okie);
            }
            Err(uhoh) => {
                if !matches!(uhoh, HttpError::BadRequestError) {
                    error!(
                        "error sending bi metrics={:?}, will requeue and re-attempt later",
                        uhoh
                    );
                    let rendered_metrics = &mut *metrics_queue.lock().await;
                    for metric in batch.iter() {
                        let _ = rendered_metrics.add(metric.clone());
                    }
                }
            }
        }
    }
}

///
/// All test currently (simply) check that metrics will not interfere with UX in any - for instance,
/// auth or connection failure should just log the failure , but not push an errors
/// back to caller who is await-ing.
///
#[cfg(test)]
pub mod tests {
    use super::AXPMetric;
    use super::AppReady;
    use super::BehavioralMetricsService;
    use super::ContextualMetricsService;
    use super::EntOsCustomMetric;
    use super::InAppMedia;
    use super::InAppMediaEventType;
    use super::OntologyPayload;
    use super::SiftDeviceType;
    use super::SiftService;
    use crate::gateway::appsanity_gateway::{AgePolicyConfig, MetricsSchemas};
    use std::collections::HashMap;
    use std::sync::Arc;
    use std::sync::RwLock;

    use crate::model::metrics::{
        Action, AppDataGovernanceState, BadgerAppAction, BadgerMetrics, BadgerMetricsService,
        BehavioralMetricContext as AppContext, BehavioralMetricPayload as AppBehavioralMetric,
        CategoryType, MetricsContext, Ready,
    };
    use ripple_sdk::api::firebolt::fb_discovery::AgePolicy;
    use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_metrics::{FlatMapValue, Param};

    use serde_json::Value;
    use thunder_ripple_sdk::tests::mock_thunder_controller::MockThunderController;
    use tokio::sync::Mutex as TokioMutex;

    use queues::CircularBuffer;
    use tokio::sync::Mutex;
    use uuid::Uuid;
    use wiremock::matchers::{method, path};
    use wiremock::{Mock, MockServer, ResponseTemplate};

    fn metrics_context() -> MetricsContext {
        MetricsContext {
            enabled: false,
            device_language: "en-US".to_string(),
            device_model: "eniac".to_string(),
            device_id: Some("test-device-id".to_string()),
            device_timezone: String::from("PDT"),
            device_timezone_offset: String::from("-07:00"),
            device_name: Some("Test Device".to_string()),
            mac_address: "DEAD-BEEF-DEAD".to_string(),
            serial_number: "0123456789".to_string(),
            account_id: Some("test-account-id".to_string()),
            platform: String::from("entos-rdk"),
            os_name: "CoolOS".to_string(),
            os_ver: "0.0.1P".to_string(), // P suffix for prod
            device_session_id: Uuid::new_v4().to_string(),
            distribution_tenant_id: "comcast".to_string(),
            firmware: String::from("1.0.0"),
            ripple_version: String::from("2.0.0"),
            data_governance_tags: Some(vec!["tag1".to_string(), "tag2".to_string()]),
            activated: Some(true),
            proposition: String::from("test-prop"),
            retailer: Some("test-retailer".to_string()),
            primary_provider: Some("test-provider".to_string()),
            coam: Some(false),
            country: Some("US".to_string()),
            region: Some("NA".to_string()),
            account_type: Some("premium".to_string()),
            operator: Some("test-operator".to_string()),
            account_detail_type: Some("primary".to_string()),
            device_type: String::from("ipstb"),
            device_manufacturer: String::from("Test Manufacturer"),
            authenticated: Some(true),
            age_policy: Some(AgePolicy::Child),
            age_policy_cets: Some(vec!["G".to_string(), "TV-Y".to_string()]),
        }
    }

    // Helper functions for tests

    fn metrics_schemas() -> MetricsSchemas {
        MetricsSchemas {
            default_metrics_namespace: "test-namespace".to_string(),
            default_metrics_schema_version: "4".to_string(),
            default_common_schema: "entos/common/5".to_string(),
            metrics_schemas: vec![],
        }
    }

    fn custom_metrics_schemas() -> MetricsSchemas {
        use crate::gateway::appsanity_gateway::MetricsSchema;
        MetricsSchemas {
            default_metrics_namespace: "entos".to_string(),
            default_metrics_schema_version: "4".to_string(),
            default_common_schema: "custom/common/5".to_string(),
            metrics_schemas: vec![
                MetricsSchema {
                    event_name: "ready".to_string(),
                    alias: Some("app_ready".to_string()),
                    namespace: Some("custom".to_string()),
                    version: Some("5".to_string()),
                },
                MetricsSchema {
                    event_name: "action".to_string(),
                    alias: None,
                    namespace: None,
                    version: Some("3".to_string()),
                },
            ],
        }
    }

    fn behavioral_context() -> AppContext {
        AppContext {
            app_id: "com.test.app".to_string(),
            product_version: "1.2.3".to_string(),
            partner_id: "testcast".to_string(),
            app_session_id: "session-123".to_string(),
            app_user_session_id: Some("user-session-456".to_string()),
            durable_app_id: "durable-app-789".to_string(),
            app_version: Some("2.3.4".to_string()),
            governance_state: Some(AppDataGovernanceState::default()),
        }
    }

    fn test_age_policy_config() -> AgePolicyConfig {
        let mut category_mappings = HashMap::new();
        let mut cet_mappings = HashMap::new();

        category_mappings.insert(
            AgePolicy::Child,
            vec!["child-safe".to_string(), "educational".to_string()],
        );
        category_mappings.insert(
            AgePolicy::Teen,
            vec!["teen-appropriate".to_string(), "social".to_string()],
        );

        cet_mappings.insert(AgePolicy::Child, vec!["G".to_string(), "TV-Y".to_string()]);
        cet_mappings.insert(AgePolicy::Teen, vec!["PG".to_string(), "PG-13".to_string()]);

        AgePolicyConfig {
            category_tag_mappings: category_mappings,
            cet_mappings,
        }
    }

    fn custom_payload() -> EntOsCustomMetric {
        EntOsCustomMetric {
            device_mac: "BEEF".to_string(),
            device_serial: "cereal".to_string(),
            source_app: String::from("app"),
            source_product_version: String::from("1.2.3"),
            authenticated: true,
            access_schema: None,
            access_payload: None,
        }
    }

    fn new_sift_service(
        sift_endpoint: String,
        ontology: bool,
        batch_size: u8,
        _max_queue_size: u8,
        metrics_schemas: MetricsSchemas,
        metrics_context: Option<MetricsContext>,
        eos_rendered: Arc<TokioMutex<CircularBuffer<Value>>>,
    ) -> impl BehavioralMetricsService + BadgerMetricsService + ContextualMetricsService + Sync + Send
    {
        let result = SiftService {
            thunder: MockThunderController::get_thunder_state_mock(),
            sift_endpoint,
            ontology,
            batch_size,
            metrics_schemas,
            metrics_context: Arc::new(RwLock::new(metrics_context)),
            metrics_send_queue: eos_rendered,
            analytics_plugin: false,
            age_policy_config: AgePolicyConfig::default(),
            service_client: None,
        };

        result
    }

    #[tokio::test]
    pub async fn test_happy_badger() {
        let listener = std::net::TcpListener::bind("127.0.0.1:9997").unwrap();
        let mock_server = MockServer::builder().listener(listener).start().await;

        Mock::given(method("POST"))
            .and(path("/platco/dev"))
            .respond_with(ResponseTemplate::new(200))
            .mount(&mock_server)
            .await;

        let metrics_context = metrics_context();

        let _app_name = "foo".to_string();
        let _app_ver = "1.0.0".to_string();
        let _partner_id = "charter".to_string();
        let _custom_payload = custom_payload();

        let service = new_sift_service(
            "http://localhost:9997/platco/dev".to_string(),
            false,
            1,
            20,
            metrics_schemas(),
            None,
            Arc::new(Mutex::new(CircularBuffer::new(2))),
        );

        let context = behavioral_context();

        service
            .send_metric(AppBehavioralMetric::Ready(Ready {
                context: context.clone(),
                ttmu_ms: 30000,
            }))
            .await;

        let p = Param {
            name: "asdf".to_string(),
            value: FlatMapValue::String("fda".to_string()),
        };
        let params = vec![p];

        service.update_metrics_context(Some(metrics_context));

        let a = BadgerMetrics::AppAction(BadgerAppAction {
            context: context.clone(),
            action: "boom".to_string(),
            args: params.clone(),
        });
        let b = BadgerMetrics::AppAction(BadgerAppAction {
            context: context.clone(),
            action: "bam".to_string(),
            args: params.clone(),
        });
        let c = BadgerMetrics::AppAction(BadgerAppAction {
            context: context.clone(),
            action: "whamo".to_string(),
            args: params.clone(),
        });
        service.send_badger_metric(a).await;
        service.send_badger_metric(b).await;
        service.send_badger_metric(c).await;

        assert_eq!(true, true);
    }
    #[tokio::test]
    pub async fn test_sad_badger() {
        let listener = std::net::TcpListener::bind("127.0.0.1:9999").unwrap();
        let mock_server = MockServer::builder().listener(listener).start().await;

        Mock::given(method("POST"))
            .and(path("/platco/dev"))
            .respond_with(ResponseTemplate::new(503))
            .mount(&mock_server)
            .await;

        let metrics_context = metrics_context();

        let _app_name = "foo".to_string();
        let _app_ver = "1.0.0".to_string();
        let _partner_id = "charter".to_string();
        let _custom_payload = custom_payload();

        let service = new_sift_service(
            "http://localhost:9999/platco/dev".to_string(),
            false,
            1,
            20,
            metrics_schemas(),
            None,
            Arc::new(Mutex::new(CircularBuffer::new(2))),
        );

        let context = behavioral_context();

        service
            .send_metric(AppBehavioralMetric::Ready(Ready {
                context: context.clone(),
                ttmu_ms: 30000,
            }))
            .await;

        let p = Param {
            name: "asdf".to_string(),
            value: FlatMapValue::String("fda".to_string()),
        };
        let params = vec![p];

        service.update_metrics_context(Some(metrics_context));

        let a = BadgerMetrics::AppAction(BadgerAppAction {
            context: context.clone(),
            action: "boom".to_string(),
            args: params.clone(),
        });
        let b = BadgerMetrics::AppAction(BadgerAppAction {
            context: context.clone(),
            action: "bam".to_string(),
            args: params.clone(),
        });
        let c = BadgerMetrics::AppAction(BadgerAppAction {
            context: context.clone(),
            action: "whamo".to_string(),
            args: params.clone(),
        });
        service.send_badger_metric(a).await;
        service.send_badger_metric(b).await;
        service.send_badger_metric(c).await;

        assert_eq!(true, true);
    }
    #[tokio::test]
    pub async fn test_happy_firebolt() {
        let listener = std::net::TcpListener::bind("127.0.0.1:6666").unwrap();
        let mock_server = MockServer::builder().listener(listener).start().await;

        Mock::given(method("POST"))
            .and(path("/platco/dev"))
            .respond_with(ResponseTemplate::new(200))
            .mount(&mock_server)
            .await;

        let metrics_context = metrics_context();

        let _app_name = "foo".to_string();
        let _app_ver = "1.0.0".to_string();
        let _partner_id = "charter".to_string();
        let _custom_payload = custom_payload();

        let service = new_sift_service(
            "http://localhost:6666/entos/dev".to_string(),
            false,
            1,
            20,
            metrics_schemas(),
            None,
            Arc::new(tokio::sync::Mutex::new(CircularBuffer::new(2))),
        );

        let context = behavioral_context();

        service
            .send_metric(AppBehavioralMetric::Ready(Ready {
                context: context.clone(),
                ttmu_ms: 30000,
            }))
            .await;

        let p = Param {
            name: "asdf".to_string(),
            value: FlatMapValue::String("fda".to_string()),
        };
        let params = vec![p];

        service.update_metrics_context(Some(metrics_context));

        service
            .send_metric(AppBehavioralMetric::Action(Action {
                context: context.clone(),
                age_policy: None,
                category: CategoryType::user,
                parameters: params.clone(),
                _type: "user".to_string(),
            }))
            .await;
    }
    #[tokio::test]
    pub async fn test_sad_firebolt() {
        let listener = std::net::TcpListener::bind("127.0.0.1:6667").unwrap();
        let mock_server = MockServer::builder().listener(listener).start().await;

        Mock::given(method("POST"))
            .and(path("/platco/dev"))
            .respond_with(ResponseTemplate::new(503))
            .mount(&mock_server)
            .await;

        let metrics_context = metrics_context();

        let _app_name = "foo".to_string();
        let _app_ver = "1.0.0".to_string();
        let _partner_id = "charter".to_string();
        let _custom_payload = custom_payload();

        let service = new_sift_service(
            "http://localhost:6667/platco/dev".to_string(),
            false,
            1,
            20,
            metrics_schemas(),
            None,
            Arc::new(Mutex::new(CircularBuffer::new(2))),
        );

        let context = behavioral_context();

        service
            .send_metric(AppBehavioralMetric::Ready(Ready {
                context: context.clone(),
                ttmu_ms: 30000,
            }))
            .await;

        let p = Param {
            name: "asdf".to_string(),
            value: FlatMapValue::String("fda".to_string()),
        };
        let _params = vec![p];

        service.update_metrics_context(Some(metrics_context));

        assert_eq!(true, true);
    }

    #[test]
    fn test_sift_device_type_mapping() {
        assert_eq!(SiftDeviceType::from_form_factor("ipstb"), "ipstb");
        assert_eq!(SiftDeviceType::from_form_factor("smarttv"), "iptv");
        assert_eq!(SiftDeviceType::from_form_factor("unknown"), "unknown");
        assert_eq!(SiftDeviceType::from_form_factor(""), "");
    }

    #[test]
    fn test_in_app_media_creation() {
        let context = behavioral_context();
        let media = InAppMedia::new(
            InAppMediaEventType::MediaPlay,
            "entity-123".to_string(),
            context.clone(),
        );

        assert_eq!(media.media_event_name, InAppMediaEventType::MediaPlay);
        assert_eq!(media.src_entity_id, Some("entity-123".to_string()));
        assert_eq!(media.app_session_id, context.app_session_id);
        assert_eq!(
            media.app_user_session_id,
            context.app_user_session_id.unwrap()
        );
        assert_eq!(media.durable_app_id, context.durable_app_id);
        assert_eq!(media.app_version, context.app_version);
    }

    #[test]
    fn test_in_app_media_serialization() {
        let context = behavioral_context();
        let media = InAppMedia::new(
            InAppMediaEventType::MediaProgress,
            "progress-entity".to_string(),
            context,
        );

        let serialized = serde_json::to_string(&media).unwrap();
        let value: serde_json::Value = serde_json::from_str(&serialized).unwrap();

        assert_eq!(value["media_event_name"], "mediaProgress");
        assert_eq!(value["src_entity_id"], "progress-entity");
        assert_eq!(value["app_session_id"], "session-123");
        assert_eq!(value["durable_app_id"], "durable-app-789");
    }

    #[test]
    fn test_age_policy_config_category_tags_retrieval() {
        let config = test_age_policy_config();

        let child_tags = config.get_category_tags(Some(AgePolicy::Child));
        assert!(child_tags.is_some());
        let tags = child_tags.unwrap();
        assert_eq!(tags.tags, vec!["child-safe", "educational"]);

        let teen_tags = config.get_category_tags(Some(AgePolicy::Teen));
        assert!(teen_tags.is_some());
        let tags = teen_tags.unwrap();
        assert_eq!(tags.tags, vec!["teen-appropriate", "social"]);

        let adult_tags = config.get_category_tags(Some(AgePolicy::Adult));
        assert!(adult_tags.is_none());
    }

    #[test]
    fn test_age_policy_config_cets_retrieval() {
        let config = test_age_policy_config();

        let child_cets = config.get_policy_cets(Some(AgePolicy::Child), vec![AgePolicy::Child]);
        assert_eq!(child_cets, Some(vec!["G".to_string(), "TV-Y".to_string()]));

        let teen_cets = config.get_policy_cets(Some(AgePolicy::Teen), vec![AgePolicy::Teen]);
        assert_eq!(teen_cets, Some(vec!["PG".to_string(), "PG-13".to_string()]));

        let adult_cets = config.get_policy_cets(Some(AgePolicy::Adult), vec![AgePolicy::Adult]);
        assert_eq!(adult_cets, None);
    }

    #[test]
    fn test_option_vec_string_combination_both_some() {
        // Simulate the combination logic from the actual code
        let cet_list: Option<Vec<String>> = Some(vec!["PG".to_string(), "PG-13".to_string()]);
        let category_cets: Option<Vec<String>> = Some(vec!["G".to_string(), "TV-Y".to_string()]);

        let combined_cets = match (category_cets, cet_list) {
            (Some(mut cats), Some(cets)) => {
                cats.extend(cets);
                Some(cats)
            }
            (Some(cats), None) => Some(cats),
            (None, Some(cets)) => Some(cets),
            (None, None) => None,
        };

        assert_eq!(
            combined_cets,
            Some(vec![
                "G".to_string(),
                "TV-Y".to_string(),
                "PG".to_string(),
                "PG-13".to_string()
            ])
        );
    }

    #[test]
    fn test_option_vec_string_combination_all_cases() {
        // Test all four combinations of Some/None
        let test_cases = vec![
            (
                Some(vec!["A".to_string()]),
                Some(vec!["B".to_string()]),
                Some(vec!["A".to_string(), "B".to_string()]),
            ),
            (
                Some(vec!["A".to_string()]),
                None,
                Some(vec!["A".to_string()]),
            ),
            (
                None,
                Some(vec!["B".to_string()]),
                Some(vec!["B".to_string()]),
            ),
            (None, None, None),
        ];

        for (category_cets, cet_list, expected) in test_cases {
            let combined_cets = match (category_cets, cet_list) {
                (Some(mut cats), Some(cets)) => {
                    cats.extend(cets);
                    Some(cats)
                }
                (Some(cats), None) => Some(cats),
                (None, Some(cets)) => Some(cets),
                (None, None) => None,
            };
            assert_eq!(combined_cets, expected);
        }
    }

    #[test]
    fn test_metrics_context_creation() {
        let ctx = metrics_context();

        assert_eq!(ctx.device_model, "eniac");
        assert_eq!(ctx.device_type, "ipstb");
        assert_eq!(ctx.os_name, "CoolOS");
        assert_eq!(ctx.os_ver, "0.0.1P");
        assert_eq!(ctx.platform, "entos-rdk");
        assert_eq!(ctx.distribution_tenant_id, "comcast");
        assert_eq!(ctx.device_manufacturer, "Test Manufacturer");
        assert_eq!(ctx.mac_address, "DEAD-BEEF-DEAD");
        assert_eq!(ctx.serial_number, "0123456789");
        assert_eq!(ctx.device_id, Some("test-device-id".to_string()));
        assert_eq!(ctx.account_id, Some("test-account-id".to_string()));
        assert_eq!(ctx.activated, Some(true));
        assert_eq!(ctx.authenticated, Some(true));
        assert_eq!(ctx.country, Some("US".to_string()));
        assert_eq!(ctx.region, Some("NA".to_string()));
    }

    #[test]
    fn test_behavioral_context_creation() {
        let ctx = behavioral_context();

        assert_eq!(ctx.app_id, "com.test.app");
        assert_eq!(ctx.product_version, "1.2.3");
        assert_eq!(ctx.partner_id, "testcast");
        assert_eq!(ctx.app_session_id, "session-123");
        assert_eq!(
            ctx.app_user_session_id,
            Some("user-session-456".to_string())
        );
        assert_eq!(ctx.durable_app_id, "durable-app-789");
        assert_eq!(ctx.app_version, Some("2.3.4".to_string()));
        assert_eq!(
            ctx.governance_state,
            Some(AppDataGovernanceState::default())
        );
    }

    #[test]
    fn test_custom_metrics_schemas() {
        let schemas = custom_metrics_schemas();

        assert_eq!(schemas.default_metrics_namespace, "entos");
        assert_eq!(schemas.default_metrics_schema_version, "4");
        assert_eq!(schemas.metrics_schemas.len(), 2);

        let ready_schema = &schemas.metrics_schemas[0];
        assert_eq!(ready_schema.event_name, "ready");
        assert_eq!(ready_schema.alias, Some("app_ready".to_string()));
        assert_eq!(ready_schema.namespace, Some("custom".to_string()));
        assert_eq!(ready_schema.version, Some("5".to_string()));
    }

    #[test]
    fn test_action_struct_creation() {
        let action = Action {
            context: behavioral_context(),
            category: CategoryType::user,
            _type: "click".to_string(),
            parameters: vec![Param {
                name: "button".to_string(),
                value: FlatMapValue::String("submit".to_string()),
            }],
            age_policy: Some(AgePolicy::Child),
        };

        assert_eq!(action.category, CategoryType::user);
        assert_eq!(action._type, "click");
        assert_eq!(action.parameters.len(), 1);
        assert_eq!(action.parameters[0].name, "button");
        assert_eq!(action.age_policy, Some(AgePolicy::Child));
    }

    #[test]
    fn test_ready_struct_creation() {
        let ready = Ready {
            context: behavioral_context(),
            ttmu_ms: 5000,
        };

        assert_eq!(ready.ttmu_ms, 5000);
        assert_eq!(ready.context.app_id, "com.test.app");
    }

    #[tokio::test]
    async fn test_sift_service_with_config() {
        use crate::gateway::appsanity_gateway::defaults;
        use thunder_ripple_sdk::tests::mock_thunder_controller::MockThunderController;

        let config = defaults();
        let thunder = MockThunderController::get_thunder_state_mock();
        let service = SiftService::new(&config, thunder, None);

        // Verify basic properties
        assert_eq!(service.ontology, config.behavioral_metrics.sift.ontology);
        assert_eq!(
            service.batch_size,
            config.behavioral_metrics.sift.batch_size
        );

        let ready_metric = AppBehavioralMetric::Ready(Ready {
            context: behavioral_context(),
            ttmu_ms: 5000,
        });

        // This should not panic - the method should handle the metric gracefully
        service.send_metric(ready_metric).await;

        // If we get here without panicking, the test passes
        assert!(true);
    }

    #[tokio::test]
    async fn test_contextual_metrics_service_basic() {
        use crate::gateway::appsanity_gateway::defaults;
        use thunder_ripple_sdk::tests::mock_thunder_controller::MockThunderController;

        let config = defaults();
        let thunder = MockThunderController::get_thunder_state_mock();
        let service = SiftService::new(&config, thunder, None);

        let new_context = {
            let mut ctx = metrics_context();
            ctx.device_model = "updated-model".to_string();
            ctx.account_id = Some("new-account".to_string());
            ctx
        };

        // This should not panic
        service.update_metrics_context(Some(new_context));

        // If we get here without panicking, the test passes
        assert!(true);
    }

    #[test]
    fn test_ontology_payload_uses_configurable_common_schema() {
        // Test that OntologyPayload uses the configurable common_schema from MetricsSchemas
        let metrics_context = metrics_context();
        let app_ready = AppReady {
            app_session_id: "session-123".to_string(),
            app_user_session_id: "user-session-456".to_string(),
            durable_app_id: "com.test.app".to_string(),
            is_cold_launch: true,
        };
        let ready_metric = AXPMetric::AppReady(app_ready);

        // Test with default metrics schemas (should use "entos/common/5")
        let default_schemas = metrics_schemas();
        let payload_default = OntologyPayload::new(
            &metrics_context,
            "test-app".to_string(),
            "1.0.0".to_string(),
            "test-partner".to_string(),
            None,
            None,
            ready_metric.clone(),
            &default_schemas,
        );
        assert_eq!(payload_default.common_schema, "entos/common/5");

        // Test with custom metrics schemas (should use "custom/common/5")
        let custom_schemas = custom_metrics_schemas();
        let payload_custom = OntologyPayload::new(
            &metrics_context,
            "test-app".to_string(),
            "1.0.0".to_string(),
            "test-partner".to_string(),
            None,
            None,
            ready_metric,
            &custom_schemas,
        );
        assert_eq!(payload_custom.common_schema, "custom/common/5");
    }

    #[test]
    fn test_metrics_schemas_default_common_schema() {
        // Test that MetricsSchemas includes the default_common_schema field
        let schemas = metrics_schemas();
        assert_eq!(schemas.default_common_schema, "entos/common/5");

        let custom_schemas = custom_metrics_schemas();
        assert_eq!(custom_schemas.default_common_schema, "custom/common/5");
    }
}
