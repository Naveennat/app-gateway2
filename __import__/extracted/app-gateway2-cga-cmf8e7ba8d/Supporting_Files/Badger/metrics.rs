use std::collections::{HashMap, HashSet};

use async_trait::async_trait;
use ripple_sdk::api::firebolt::fb_discovery::AgePolicy;
use serde::{Deserialize, Serialize};
use serde_json::Number;
use serde_json::Value;

use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_metrics::ErrorType;
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_metrics::FlatMapValue;
use thunder_ripple_sdk::ripple_sdk::api::{
    firebolt::fb_metrics::{AppLifecycleState, AppLifecycleStateChange},
    gateway::rpc_gateway_api::CallContext,
};

use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_metrics::Param;

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct BehavioralMetricContext {
    pub app_id: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub app_version: Option<String>,
    pub product_version: String,
    pub partner_id: String,
    pub app_session_id: String,
    pub app_user_session_id: Option<String>,
    pub durable_app_id: String,
    pub governance_state: Option<AppDataGovernanceState>,
}

impl From<CallContext> for BehavioralMetricContext {
    fn from(call_context: CallContext) -> Self {
        BehavioralMetricContext {
            app_id: call_context.app_id.clone(),
            product_version: String::from("product.version.not.implemented"),
            partner_id: String::from("partner.id.not.set"),
            app_session_id: String::from("app_session_id.not.set"),
            durable_app_id: call_context.app_id,
            app_version: None,
            app_user_session_id: None,
            governance_state: None,
        }
    }
}

impl From<&str> for BehavioralMetricContext {
    fn from(app_id: &str) -> Self {
        BehavioralMetricContext {
            app_id: app_id.to_owned(),
            product_version: String::from("product.version.not.implemented"),
            partner_id: String::from("partner.id.not.set"),
            app_session_id: String::from("app_session_id.not.set"),
            durable_app_id: app_id.to_owned(),
            app_version: None,
            app_user_session_id: None,
            governance_state: None,
        }
    }
}

#[async_trait]
pub trait MetricsContextProvider: core::fmt::Debug {
    async fn provide_context(&mut self) -> Option<MetricsContext>;
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct Ready {
    pub context: BehavioralMetricContext,
    pub ttmu_ms: u32,
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub enum MediaPositionType {
    None,
    PercentageProgress(f32),
    AbsolutePosition(i32),
}
impl MediaPositionType {
    pub fn as_absolute(self) -> Option<i32> {
        match self {
            MediaPositionType::None => None,
            MediaPositionType::PercentageProgress(_) => None,
            MediaPositionType::AbsolutePosition(absolute) => Some(absolute),
        }
    }
    pub fn as_percentage(self) -> Option<f32> {
        match self {
            MediaPositionType::None => None,
            MediaPositionType::PercentageProgress(percentage) => Some(percentage),
            MediaPositionType::AbsolutePosition(_) => None,
        }
    }
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct SignIn {
    pub context: BehavioralMetricContext,
}
#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct SignOut {
    pub context: BehavioralMetricContext,
}
#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct StartContent {
    pub context: BehavioralMetricContext,
    pub entity_id: Option<String>,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}

impl AgePolicyAccessor for StartContent {
    fn get_age_policy(&self) -> &Option<AgePolicy> {
        &self.age_policy
    }

    fn set_age_policy(&mut self, age_policy: Option<AgePolicy>) {
        self.age_policy = age_policy;
    }
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct StopContent {
    pub context: BehavioralMetricContext,
    pub entity_id: Option<String>,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}

impl AgePolicyAccessor for StopContent {
    fn get_age_policy(&self) -> &Option<AgePolicy> {
        &self.age_policy
    }

    fn set_age_policy(&mut self, age_policy: Option<AgePolicy>) {
        self.age_policy = age_policy;
    }
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct Page {
    pub context: BehavioralMetricContext,
    pub page_id: String,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}

impl AgePolicyAccessor for Page {
    fn get_age_policy(&self) -> &Option<AgePolicy> {
        &self.age_policy
    }

    fn set_age_policy(&mut self, age_policy: Option<AgePolicy>) {
        self.age_policy = age_policy;
    }
}

#[allow(non_camel_case_types)]
#[derive(Debug, Serialize, Deserialize, Clone)]
pub enum ActionType {
    user,
    app,
}
#[allow(non_camel_case_types)]
#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub enum CategoryType {
    user,
    app,
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct Action {
    pub context: BehavioralMetricContext,
    pub category: CategoryType,
    #[serde(rename = "type")]
    pub _type: String,
    pub parameters: Vec<Param>,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}

impl AgePolicyAccessor for Action {
    fn get_age_policy(&self) -> &Option<AgePolicy> {
        &self.age_policy
    }

    fn set_age_policy(&mut self, age_policy: Option<AgePolicy>) {
        self.age_policy = age_policy;
    }
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct MetricsError {
    pub context: BehavioralMetricContext,
    #[serde(alias = "type")]
    pub error_type: ErrorType,
    pub code: String,
    pub description: String,
    pub visible: bool,
    pub parameters: Option<HashMap<String, FlatMapValue>>,
    pub durable_app_id: String,
    pub third_party_error: bool,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}
#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct MediaLoadStart {
    pub context: BehavioralMetricContext,
    pub entity_id: String,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}

impl AgePolicyAccessor for MediaLoadStart {
    fn get_age_policy(&self) -> &Option<AgePolicy> {
        &self.age_policy
    }

    fn set_age_policy(&mut self, age_policy: Option<AgePolicy>) {
        self.age_policy = age_policy;
    }
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct MediaPlay {
    pub context: BehavioralMetricContext,
    pub entity_id: String,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}

impl AgePolicyAccessor for MediaPlay {
    fn get_age_policy(&self) -> &Option<AgePolicy> {
        &self.age_policy
    }

    fn set_age_policy(&mut self, age_policy: Option<AgePolicy>) {
        self.age_policy = age_policy;
    }
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct MediaPlaying {
    pub context: BehavioralMetricContext,
    pub entity_id: String,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}

impl AgePolicyAccessor for MediaPlaying {
    fn get_age_policy(&self) -> &Option<AgePolicy> {
        &self.age_policy
    }

    fn set_age_policy(&mut self, age_policy: Option<AgePolicy>) {
        self.age_policy = age_policy;
    }
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct MediaPause {
    pub context: BehavioralMetricContext,
    pub entity_id: String,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}

impl AgePolicyAccessor for MediaPause {
    fn get_age_policy(&self) -> &Option<AgePolicy> {
        &self.age_policy
    }

    fn set_age_policy(&mut self, age_policy: Option<AgePolicy>) {
        self.age_policy = age_policy;
    }
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct MediaWaiting {
    pub context: BehavioralMetricContext,
    pub entity_id: String,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}

impl AgePolicyAccessor for MediaWaiting {
    fn get_age_policy(&self) -> &Option<AgePolicy> {
        &self.age_policy
    }

    fn set_age_policy(&mut self, age_policy: Option<AgePolicy>) {
        self.age_policy = age_policy;
    }
}
#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct MediaProgress {
    pub context: BehavioralMetricContext,
    pub entity_id: String,
    pub progress: Option<MediaPositionType>,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}

impl AgePolicyAccessor for MediaProgress {
    fn get_age_policy(&self) -> &Option<AgePolicy> {
        &self.age_policy
    }

    fn set_age_policy(&mut self, age_policy: Option<AgePolicy>) {
        self.age_policy = age_policy;
    }
}

impl From<&MediaProgress> for Option<i32> {
    fn from(progress: &MediaProgress) -> Self {
        match progress.progress.clone() {
            Some(prog) => prog.as_absolute(),
            None => None,
        }
    }
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct MediaSeeking {
    pub context: BehavioralMetricContext,
    pub entity_id: String,
    pub target: Option<MediaPositionType>,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}

impl AgePolicyAccessor for MediaSeeking {
    fn get_age_policy(&self) -> &Option<AgePolicy> {
        &self.age_policy
    }

    fn set_age_policy(&mut self, age_policy: Option<AgePolicy>) {
        self.age_policy = age_policy;
    }
}

impl From<&MediaSeeking> for Option<i32> {
    fn from(progress: &MediaSeeking) -> Self {
        match progress.target.clone() {
            Some(prog) => prog.as_absolute(),
            None => None,
        }
    }
}

impl From<&MediaSeeking> for Option<f32> {
    fn from(progress: &MediaSeeking) -> Self {
        match progress.target.clone() {
            Some(prog) => prog.as_percentage(),
            None => None,
        }
    }
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct MediaSeeked {
    pub context: BehavioralMetricContext,
    pub entity_id: String,
    pub position: Option<MediaPositionType>,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}

impl AgePolicyAccessor for MediaSeeked {
    fn get_age_policy(&self) -> &Option<AgePolicy> {
        &self.age_policy
    }

    fn set_age_policy(&mut self, age_policy: Option<AgePolicy>) {
        self.age_policy = age_policy;
    }
}

impl From<&MediaSeeked> for Option<i32> {
    fn from(progress: &MediaSeeked) -> Self {
        match progress.position.clone() {
            Some(prog) => prog.as_absolute(),
            None => None,
        }
    }
}

impl From<&MediaSeeked> for Option<f32> {
    fn from(progress: &MediaSeeked) -> Self {
        match progress.position.clone() {
            Some(prog) => prog.as_percentage(),
            None => None,
        }
    }
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct MediaRateChanged {
    pub context: BehavioralMetricContext,
    pub entity_id: String,
    pub rate: Number,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}

impl AgePolicyAccessor for MediaRateChanged {
    fn get_age_policy(&self) -> &Option<AgePolicy> {
        &self.age_policy
    }

    fn set_age_policy(&mut self, age_policy: Option<AgePolicy>) {
        self.age_policy = age_policy;
    }
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct MediaRenditionChanged {
    pub context: BehavioralMetricContext,
    pub entity_id: String,
    pub bitrate: Number,
    pub width: u32,
    pub height: u32,
    pub profile: Option<String>,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}

impl AgePolicyAccessor for MediaRenditionChanged {
    fn get_age_policy(&self) -> &Option<AgePolicy> {
        &self.age_policy
    }

    fn set_age_policy(&mut self, age_policy: Option<AgePolicy>) {
        self.age_policy = age_policy;
    }
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct MediaEnded {
    pub context: BehavioralMetricContext,
    pub entity_id: String,
    #[serde(rename = "agePolicy")]
    pub age_policy: Option<AgePolicy>,
}

impl AgePolicyAccessor for MediaEnded {
    fn get_age_policy(&self) -> &Option<AgePolicy> {
        &self.age_policy
    }

    fn set_age_policy(&mut self, age_policy: Option<AgePolicy>) {
        self.age_policy = age_policy;
    }
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct RawBehaviorMetricRequest {
    pub context: BehavioralMetricContext,
    pub value: Value,
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub enum BehavioralMetricPayload {
    Ready(Ready),
    SignIn(SignIn),
    SignOut(SignOut),
    StartContent(StartContent),
    StopContent(StopContent),
    Page(Page),
    Action(Action),
    Error(MetricsError),
    MediaLoadStart(MediaLoadStart),
    MediaPlay(MediaPlay),
    MediaPlaying(MediaPlaying),
    MediaPause(MediaPause),
    MediaWaiting(MediaWaiting),
    MediaProgress(MediaProgress),
    MediaSeeking(MediaSeeking),
    MediaSeeked(MediaSeeked),
    MediaRateChanged(MediaRateChanged),
    MediaRenditionChanged(MediaRenditionChanged),
    MediaEnded(MediaEnded),
    AppStateChange(SiftAppLifecycleStateChange),
    Raw(RawBehaviorMetricRequest),
}

impl IUpdateContext for BehavioralMetricPayload {
    fn update_context(&mut self, context: BehavioralMetricContext) {
        match self {
            Self::Ready(r) => r.context = context,
            Self::SignIn(s) => s.context = context,
            Self::SignOut(s) => s.context = context,
            Self::StartContent(s) => s.context = context,
            Self::StopContent(s) => s.context = context,
            Self::Page(p) => p.context = context,
            Self::Action(a) => a.context = context,
            Self::Error(e) => e.context = context,
            Self::MediaLoadStart(m) => m.context = context,
            Self::MediaPlay(m) => m.context = context,
            Self::MediaPlaying(m) => m.context = context,
            Self::MediaPause(m) => m.context = context,
            Self::MediaWaiting(m) => m.context = context,
            Self::MediaProgress(m) => m.context = context,
            Self::MediaSeeking(m) => m.context = context,
            Self::MediaSeeked(m) => m.context = context,
            Self::MediaRateChanged(m) => m.context = context,
            Self::MediaRenditionChanged(m) => m.context = context,
            Self::MediaEnded(m) => m.context = context,
            Self::AppStateChange(a) => a.context = context,
            Self::Raw(r) => r.context = context,
        }
    }

    fn get_context(&self) -> BehavioralMetricContext {
        match self {
            Self::Ready(r) => r.context.clone(),
            Self::SignIn(s) => s.context.clone(),
            Self::SignOut(s) => s.context.clone(),
            Self::StartContent(s) => s.context.clone(),
            Self::StopContent(s) => s.context.clone(),
            Self::Page(p) => p.context.clone(),
            Self::Action(a) => a.context.clone(),
            Self::Error(e) => e.context.clone(),
            Self::MediaLoadStart(m) => m.context.clone(),
            Self::MediaPlay(m) => m.context.clone(),
            Self::MediaPlaying(m) => m.context.clone(),
            Self::MediaPause(m) => m.context.clone(),
            Self::MediaWaiting(m) => m.context.clone(),
            Self::MediaProgress(m) => m.context.clone(),
            Self::MediaSeeking(m) => m.context.clone(),
            Self::MediaSeeked(m) => m.context.clone(),
            Self::MediaRateChanged(m) => m.context.clone(),
            Self::MediaRenditionChanged(m) => m.context.clone(),
            Self::MediaEnded(m) => m.context.clone(),
            Self::AppStateChange(a) => a.context.clone(),
            Self::Raw(r) => r.context.clone(),
        }
    }
}

/// Runtime age policy extraction for BehavioralMetricPayload
impl BehavioralMetricPayload {
    /// Extract age_policy at runtime if the variant supports it
    pub fn extract_age_policy(&self) -> Option<AgePolicy> {
        match self {
            Self::StartContent(m) => m.age_policy.clone(),
            Self::StopContent(m) => m.age_policy.clone(),
            Self::Page(m) => m.age_policy.clone(),
            Self::Action(m) => m.age_policy.clone(),
            Self::MediaLoadStart(m) => m.age_policy.clone(),
            Self::MediaPlay(m) => m.age_policy.clone(),
            Self::MediaPlaying(m) => m.age_policy.clone(),
            Self::MediaPause(m) => m.age_policy.clone(),
            Self::MediaWaiting(m) => m.age_policy.clone(),
            Self::MediaProgress(m) => m.age_policy.clone(),
            Self::MediaSeeking(m) => m.age_policy.clone(),
            Self::MediaSeeked(m) => m.age_policy.clone(),
            Self::MediaRateChanged(m) => m.age_policy.clone(),
            Self::MediaRenditionChanged(m) => m.age_policy.clone(),
            Self::MediaEnded(m) => m.age_policy.clone(),
            // These variants don't support age_policy
            Self::Ready(_)
            | Self::SignIn(_)
            | Self::SignOut(_)
            | Self::Error(_)
            | Self::AppStateChange(_)
            | Self::Raw(_) => None,
        }
    }

    /// Set age_policy at runtime if the variant supports it
    pub fn set_age_policy(&mut self, policy: Option<AgePolicy>) {
        match self {
            Self::StartContent(m) => m.age_policy = policy,
            Self::StopContent(m) => m.age_policy = policy,
            Self::Page(m) => m.age_policy = policy,
            Self::Action(m) => m.age_policy = policy,
            Self::MediaLoadStart(m) => m.age_policy = policy,
            Self::MediaPlay(m) => m.age_policy = policy,
            Self::MediaPlaying(m) => m.age_policy = policy,
            Self::MediaPause(m) => m.age_policy = policy,
            Self::MediaWaiting(m) => m.age_policy = policy,
            Self::MediaProgress(m) => m.age_policy = policy,
            Self::MediaSeeking(m) => m.age_policy = policy,
            Self::MediaSeeked(m) => m.age_policy = policy,
            Self::MediaRateChanged(m) => m.age_policy = policy,
            Self::MediaRenditionChanged(m) => m.age_policy = policy,
            Self::MediaEnded(m) => m.age_policy = policy,
            // These variants don't support age_policy, so we ignore the request
            Self::Ready(_)
            | Self::SignIn(_)
            | Self::SignOut(_)
            | Self::Error(_)
            | Self::AppStateChange(_)
            | Self::Raw(_) => {
                // Optionally log that age_policy is not supported for this variant
                // eprintln!("Warning: age_policy not supported for this metric type");
            }
        }
    }

    /// Check if the variant supports age_policy
    pub fn supports_age_policy(&self) -> bool {
        match self {
            Self::StartContent(_)
            | Self::StopContent(_)
            | Self::Page(_)
            | Self::Action(_)
            | Self::MediaLoadStart(_)
            | Self::MediaPlay(_)
            | Self::MediaPlaying(_)
            | Self::MediaPause(_)
            | Self::MediaWaiting(_)
            | Self::MediaProgress(_)
            | Self::MediaSeeking(_)
            | Self::MediaSeeked(_)
            | Self::MediaRateChanged(_)
            | Self::MediaRenditionChanged(_)
            | Self::MediaEnded(_) => true,
            Self::Ready(_)
            | Self::SignIn(_)
            | Self::SignOut(_)
            | Self::Error(_)
            | Self::AppStateChange(_)
            | Self::Raw(_) => false,
        }
    }

    /// Check if age_policy is set (only for variants that support it)
    pub fn has_age_policy(&self) -> bool {
        self.extract_age_policy().is_some()
    }

    /// Get age_policy as string or empty string if None/unsupported
    pub fn get_age_policy(&self) -> Option<AgePolicy> {
        self.extract_age_policy()
    }
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct SiftAppLifecycleStateChange {
    pub context: BehavioralMetricContext,
    pub previous_state: Option<AppLifecycleState>,
    pub new_state: AppLifecycleState,
}

impl From<AppLifecycleStateChange> for SiftAppLifecycleStateChange {
    fn from(value: AppLifecycleStateChange) -> Self {
        let mut call_context = CallContext::default();
        call_context.app_id = value.app_id;
        Self {
            context: call_context.into(),
            previous_state: value.previous_state,
            new_state: value.new_state,
        }
    }
}

/// all the things that are provided by platform that need to
/// be updated, and eventually in/outjected into/out of a payload
/// These items may (or may not) be available when the ripple
/// process starts, so this service may need a way to wait for the values
/// to become available
/// This design assumes that all of the items will be available at the same times
#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct MetricsContext {
    pub enabled: bool,
    pub device_language: String,
    pub device_model: String,
    pub device_id: Option<String>,
    pub account_id: Option<String>,
    pub device_timezone: String,
    pub device_timezone_offset: String,
    pub device_name: Option<String>,
    pub platform: String,
    pub os_name: String,
    pub os_ver: String,
    pub distribution_tenant_id: String,
    pub device_session_id: String,
    pub mac_address: String,
    pub serial_number: String,
    pub firmware: String,
    pub ripple_version: String,
    pub data_governance_tags: Option<Vec<String>>,
    pub age_policy: Option<AgePolicy>,
    pub age_policy_cets: Option<Vec<String>>,
    pub activated: Option<bool>,
    pub proposition: String,
    pub retailer: Option<String>,
    pub primary_provider: Option<String>,
    pub coam: Option<bool>,
    pub country: Option<String>,
    pub region: Option<String>,
    pub account_type: Option<String>,
    pub operator: Option<String>,
    pub account_detail_type: Option<String>,
    pub device_type: String,
    pub device_manufacturer: String,
    pub authenticated: Option<bool>,
}

#[allow(non_camel_case_types)]
pub enum MetricsContextField {
    enabled,
    device_language,
    device_model,
    device_id,
    account_id,
    device_timezone,
    device_timezone_offset,
    device_name,
    platform,
    os_name,
    os_ver,
    distributor_id,
    session_id,
    mac_address,
    serial_number,
    firmware,
    ripple_version,
    env,
    age_policy,
    age_policy_cets,
    activated,
    proposition,
    retailer,
    primary_provider,
    coam,
    country,
    region,
    account_type,
    operator,
    account_detail_type,
}

impl MetricsContext {
    pub fn new() -> MetricsContext {
        MetricsContext {
            enabled: false,
            device_language: String::from(""),
            device_model: String::from(""),
            device_id: None,
            device_timezone: String::from(""),
            device_timezone_offset: String::from(""),
            device_name: None,
            mac_address: String::from(""),
            serial_number: String::from(""),
            account_id: None,
            platform: String::from("entos-rdk"),
            os_name: String::from(""),
            os_ver: String::from(""),
            device_session_id: String::from(""),
            distribution_tenant_id: String::from(""),
            firmware: String::from(""),
            ripple_version: String::from(""),
            data_governance_tags: None,
            activated: None,
            proposition: String::from(""),
            retailer: None,
            primary_provider: None,
            coam: None,
            country: None,
            region: None,
            account_type: None,
            operator: None,
            account_detail_type: None,
            device_type: String::from(""),
            device_manufacturer: String::from(""),
            authenticated: None,
            age_policy: None,
            age_policy_cets: None,
        }
    }
}
#[async_trait]
pub trait MetricsManager: Send + Sync {
    async fn send_metric(&mut self, metrics: BehavioralMetricPayload) -> ();
}
#[derive(Debug, PartialEq, Serialize, Deserialize, Clone, Default)]
pub struct AppDataGovernanceState {
    pub data_tags_to_apply: HashSet<String>,
}
impl AppDataGovernanceState {
    pub fn new(tags: HashSet<String>) -> AppDataGovernanceState {
        AppDataGovernanceState {
            data_tags_to_apply: tags,
        }
    }
}

pub trait IUpdateContext {
    fn update_context(&mut self, context: BehavioralMetricContext);
    fn get_context(&self) -> BehavioralMetricContext;
}

/// Trait for accessing age policy information from metrics structs
pub trait AgePolicyAccessor {
    /// Get the age policy value
    fn get_age_policy(&self) -> &Option<AgePolicy>;

    /// Set the age policy value
    fn set_age_policy(&mut self, age_policy: Option<AgePolicy>);

    /// Check if age policy is set
    fn has_age_policy(&self) -> bool {
        self.get_age_policy().is_some()
    }

    /// Get age policy as a string, returning empty string if None
    fn get_age_policy_or_empty(&self) -> String {
        self.get_age_policy()
            .as_ref()
            .map(|policy| policy.to_string())
            .unwrap_or_default()
    }
}

/// Simple generic accessor: if the struct implements AgePolicyAccessor, get the age_policy
pub fn get_age_policy_if_supported<T: AgePolicyAccessor>(item: &T) -> Option<AgePolicy> {
    item.get_age_policy().clone()
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct BadgerMetric {
    #[serde(skip_serializing)]
    pub context: BehavioralMetricContext,
    pub segment: Option<String>,
    pub args: Option<Vec<Param>>,
}
#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct BadgerAppAction {
    #[serde(skip_serializing)]
    pub context: BehavioralMetricContext,
    pub action: String,
    pub args: Vec<Param>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct BadgerError {
    #[serde(skip_serializing)]
    pub context: BehavioralMetricContext,
    pub message: String,
    pub visible: bool,
    pub code: u16,
    pub args: Option<Vec<Param>>,
}
#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct BadgerLaunchCompleted {
    pub context: BehavioralMetricContext,
    pub args: Option<Vec<Param>>,
}
#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct BadgerDismissLoadingScreen {
    pub context: BehavioralMetricContext,
    pub args: Option<Vec<Param>>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct BadgerPageView {
    pub context: BehavioralMetricContext,
    pub page: String,
    pub args: Option<Vec<Param>>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct BadgerUserAction {
    pub context: BehavioralMetricContext,
    pub args: Option<Vec<Param>>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct BadgerUserError {
    #[serde(skip_serializing)]
    pub context: BehavioralMetricContext,
    pub message: String,
    pub visible: bool,
    pub code: u16,
    pub args: Option<Vec<Param>>,
}
#[derive(Debug, Serialize, Deserialize, Clone)]
pub enum BadgerMetrics {
    Metric(BadgerMetric),
    AppAction(BadgerAppAction),
    Error(BadgerError),
    LaunchCompleted(BadgerLaunchCompleted),
    DismissLoadingScreen(BadgerDismissLoadingScreen),
    PageView(BadgerPageView),
    UserAction(BadgerUserAction),
    UserError(BadgerUserError),
}

impl IUpdateContext for BadgerMetrics {
    fn update_context(&mut self, context: BehavioralMetricContext) {
        match self {
            Self::Metric(metrics) => metrics.context = context,
            Self::AppAction(metrics) => metrics.context = context,
            Self::Error(metrics) => metrics.context = context,
            Self::LaunchCompleted(metrics) => metrics.context = context,
            Self::DismissLoadingScreen(metrics) => metrics.context = context,
            Self::PageView(metrics) => metrics.context = context,
            Self::UserAction(metrics) => metrics.context = context,
            Self::UserError(metrics) => metrics.context = context,
        }
    }

    fn get_context(&self) -> BehavioralMetricContext {
        match self {
            Self::Metric(metrics) => metrics.context.clone(),
            Self::AppAction(metrics) => metrics.context.clone(),
            Self::Error(metrics) => metrics.context.clone(),
            Self::LaunchCompleted(metrics) => metrics.context.clone(),
            Self::DismissLoadingScreen(metrics) => metrics.context.clone(),
            Self::PageView(metrics) => metrics.context.clone(),
            Self::UserAction(metrics) => metrics.context.clone(),
            Self::UserError(metrics) => metrics.context.clone(),
        }
    }
}

#[async_trait]
pub trait BadgerMetricsService {
    async fn send_badger_metric(&self, metrics: BadgerMetrics) -> ();
}

pub struct LoggingBehavioralMetricsManager {
    pub metrics_context: Option<MetricsContext>,
    pub log_fn: LogFn,
}
#[derive(Debug, Serialize, Deserialize)]
struct LoggableBehavioralMetric {
    context: Option<MetricsContext>,
    payload: BehavioralMetricPayload,
}
#[derive(Debug, Serialize, Deserialize)]
struct LoggableBadgerMetric {
    context: Option<MetricsContext>,
    payload: BadgerMetrics,
}
type LogFn = fn(&Value) -> ();
impl LoggingBehavioralMetricsManager {
    pub fn new(
        metrics_context: Option<MetricsContext>,
        log_fn: LogFn,
    ) -> LoggingBehavioralMetricsManager {
        LoggingBehavioralMetricsManager {
            metrics_context,
            log_fn,
        }
    }
}
impl LoggingBehavioralMetricsManager {
    pub async fn send_metric(&mut self, metrics: BehavioralMetricPayload) {
        (self.log_fn)(
            &serde_json::to_value(&LoggableBehavioralMetric {
                context: self.metrics_context.clone(),
                payload: metrics,
            })
            .unwrap_or_default(),
        );
    }
    pub async fn send_badger_metric(&mut self, metrics: BadgerMetrics) {
        (self.log_fn)(
            &serde_json::to_value(&LoggableBadgerMetric {
                context: self.metrics_context.clone(),
                payload: metrics,
            })
            .unwrap_or_default(),
        );
    }
}

/// Runtime age policy extraction examples
pub mod runtime_age_policy_examples {
    use super::*;

    /// Process any BehavioralMetricPayload and extract age_policy at runtime
    pub fn process_payload_age_policy(payload: &BehavioralMetricPayload) -> Option<AgePolicy> {
        // Extract age_policy if the variant supports it
        payload.extract_age_policy()
    }

    /// Set age policy on any payload at runtime (only if variant supports it)
    pub fn set_payload_age_policy(
        payload: &mut BehavioralMetricPayload,
        policy: Option<AgePolicy>,
    ) {
        payload.set_age_policy(policy);
    }

    /// Example function that works with any payload at runtime
    pub fn log_payload_age_policy(payload: &BehavioralMetricPayload, name: &str) {
        if payload.supports_age_policy() {
            if let Some(policy) = payload.extract_age_policy() {
                println!("{}: Age Policy = {}", name, policy);
            } else {
                println!("{}: No age policy set", name);
            }
        } else {
            println!("{}: Does not support age policy", name);
        }
    }
}

#[cfg(test)]
pub mod tests {
    use super::*;
    use ripple_sdk::api::firebolt::fb_discovery::AgePolicy;
    use std::collections::HashSet;

    #[test]
    fn test_behavioral_metric_payload() {
        let behavioral_metric_context = BehavioralMetricContext {
            app_id: "test_app_id".to_string(),
            product_version: "test_product_version".to_string(),
            partner_id: "test_partner_id".to_string(),
            app_session_id: "test_app_session_id".to_string(),
            app_user_session_id: Some("test_user_session_id".to_string()),
            durable_app_id: "test_durable_app_id".to_string(),
            app_version: Some("test_app_version".to_string()),
            governance_state: Some(AppDataGovernanceState {
                data_tags_to_apply: HashSet::new(),
            }),
        };

        let ready_payload = Ready {
            context: behavioral_metric_context.clone(),
            ttmu_ms: 100,
        };

        let mut behavioral_metric_payload = BehavioralMetricPayload::Ready(ready_payload);

        assert_eq!(
            behavioral_metric_payload.get_context(),
            behavioral_metric_context
        );

        let new_behavioral_metric_context = BehavioralMetricContext {
            app_id: "new_test_app_id".to_string(),
            product_version: "new_test_product_version".to_string(),
            partner_id: "new_test_partner_id".to_string(),
            app_session_id: "new_test_app_session_id".to_string(),
            app_user_session_id: Some("new_test_user_session_id".to_string()),
            durable_app_id: "new_test_durable_app_id".to_string(),
            app_version: Some("test_app_version".to_string()),
            governance_state: Some(AppDataGovernanceState {
                data_tags_to_apply: HashSet::new(),
            }),
        };

        behavioral_metric_payload.update_context(new_behavioral_metric_context.clone());

        assert_eq!(
            behavioral_metric_payload.get_context(),
            new_behavioral_metric_context
        );
    }

    #[test]
    fn test_runtime_age_policy_extraction() {
        let context = BehavioralMetricContext::from("test_app");

        // Test with a payload that supports age_policy
        let start_content_payload = BehavioralMetricPayload::StartContent(StartContent {
            context: context.clone(),
            entity_id: Some("movie_123".to_string()),
            age_policy: Some(AgePolicy::Teen),
        });

        // Test extraction
        assert!(start_content_payload.supports_age_policy());
        assert_eq!(
            start_content_payload.extract_age_policy(),
            Some(AgePolicy::Teen)
        );
        assert!(start_content_payload.has_age_policy());

        // Test with a payload that doesn't support age_policy
        let ready_payload = BehavioralMetricPayload::Ready(Ready {
            context: context.clone(),
            ttmu_ms: 100,
        });

        assert!(!ready_payload.supports_age_policy());
        assert_eq!(ready_payload.extract_age_policy(), None);
        assert!(!ready_payload.has_age_policy());
    }

    #[test]
    fn test_runtime_age_policy_setting() {
        let context = BehavioralMetricContext::from("test_app");

        // Test setting age_policy on supported variant
        let mut page_payload = BehavioralMetricPayload::Page(Page {
            context: context.clone(),
            page_id: "home".to_string(),
            age_policy: None,
        });

        assert!(!page_payload.has_age_policy());
        page_payload.set_age_policy(Some(AgePolicy::Adult));
        assert_eq!(page_payload.extract_age_policy(), Some(AgePolicy::Adult));

        // Test setting age_policy on unsupported variant (should be ignored)
        let mut ready_payload = BehavioralMetricPayload::Ready(Ready {
            context,
            ttmu_ms: 100,
        });

        ready_payload.set_age_policy(Some(AgePolicy::Teen)); // This should be ignored
        assert_eq!(ready_payload.extract_age_policy(), None); // Still None
    }

    // JSON Serialization/Deserialization Tests for Firebolt OpenRPC Schema Compliance

    #[test]
    fn test_behavioral_metric_context_json_serialization() {
        let context = BehavioralMetricContext {
            app_id: "test-app-id".to_string(),
            app_version: Some("1.0.0".to_string()),
            product_version: "product-1.0".to_string(),
            partner_id: "test-partner".to_string(),
            app_session_id: "session-123".to_string(),
            app_user_session_id: Some("user-session-456".to_string()),
            durable_app_id: "durable-app-id".to_string(),
            governance_state: Some(AppDataGovernanceState {
                data_tags_to_apply: ["child".to_string(), "family".to_string()]
                    .into_iter()
                    .collect(),
            }),
        };

        // Test serialization
        let json = serde_json::to_string(&context).expect("Should serialize");
        assert!(json.contains("test-app-id"));
        assert!(json.contains("1.0.0"));
        assert!(json.contains("session-123"));

        // Test deserialization
        let deserialized: BehavioralMetricContext =
            serde_json::from_str(&json).expect("Should deserialize");
        assert_eq!(deserialized, context);

        // Test with minimal fields (optional fields as None)
        let minimal_context = BehavioralMetricContext {
            app_id: "minimal-app".to_string(),
            app_version: None,
            product_version: "minimal-product".to_string(),
            partner_id: "minimal-partner".to_string(),
            app_session_id: "minimal-session".to_string(),
            app_user_session_id: None,
            durable_app_id: "minimal-durable".to_string(),
            governance_state: None,
        };

        let minimal_json =
            serde_json::to_string(&minimal_context).expect("Should serialize minimal");
        let minimal_deserialized: BehavioralMetricContext =
            serde_json::from_str(&minimal_json).expect("Should deserialize minimal");
        assert_eq!(minimal_deserialized, minimal_context);
    }

    #[test]
    fn test_ready_json_serialization() {
        let context = BehavioralMetricContext::from("test-app");
        let ready = Ready {
            context,
            ttmu_ms: 5000,
        };

        // Test serialization
        let json = serde_json::to_string(&ready).expect("Should serialize");
        assert!(json.contains("5000"));

        // Test deserialization
        let deserialized: Ready = serde_json::from_str(&json).expect("Should deserialize");
        assert_eq!(deserialized, ready);

        // Test negative case - invalid JSON structure
        let invalid_json = r#"{"context": {}, "ttmu_ms": "invalid"}"#;
        assert!(serde_json::from_str::<Ready>(invalid_json).is_err());
    }

    #[test]
    fn test_sign_in_out_json_serialization() {
        let context = BehavioralMetricContext::from("test-app");

        let sign_in = SignIn {
            context: context.clone(),
        };
        let sign_out = SignOut { context };

        // Test SignIn
        let sign_in_json = serde_json::to_string(&sign_in).expect("Should serialize SignIn");
        let sign_in_deserialized: SignIn =
            serde_json::from_str(&sign_in_json).expect("Should deserialize SignIn");
        assert_eq!(sign_in_deserialized, sign_in);

        // Test SignOut
        let sign_out_json = serde_json::to_string(&sign_out).expect("Should serialize SignOut");
        let sign_out_deserialized: SignOut =
            serde_json::from_str(&sign_out_json).expect("Should deserialize SignOut");
        assert_eq!(sign_out_deserialized, sign_out);
    }

    #[test]
    fn test_start_content_json_serialization() {
        let context = BehavioralMetricContext::from("test-app");

        // Test with all fields
        let start_content = StartContent {
            context: context.clone(),
            entity_id: Some("entity-123".to_string()),
            age_policy: Some(AgePolicy::Child),
        };

        let json = serde_json::to_string(&start_content).expect("Should serialize");
        assert!(json.contains("entity-123"));

        let deserialized: StartContent = serde_json::from_str(&json).expect("Should deserialize");
        assert_eq!(deserialized, start_content);

        // Test with minimal fields (matching Firebolt API spec)
        let minimal_start_content = StartContent {
            context,
            entity_id: None,
            age_policy: None,
        };

        let minimal_json =
            serde_json::to_string(&minimal_start_content).expect("Should serialize minimal");
        let minimal_deserialized: StartContent =
            serde_json::from_str(&minimal_json).expect("Should deserialize minimal");
        assert_eq!(minimal_deserialized, minimal_start_content);

        // Test Firebolt spec compliance - example from OpenRPC
        let firebolt_example_json = r#"{
            "context": {
                "app_id": "test-app",
                "product_version": "product.version.not.implemented",
                "partner_id": "partner.id.not.set",
                "app_session_id": "app_session_id.not.set",
                "durable_app_id": "test-app"
            },
            "entity_id": "abc",
            "age_policy": "Child"
        }"#;

        let from_firebolt: Result<StartContent, _> = serde_json::from_str(firebolt_example_json);
        assert!(
            from_firebolt.is_ok()
                || serde_json::from_str::<StartContent>(
                    &firebolt_example_json.replace("Child", "app:child")
                )
                .is_ok()
        );
    }

    #[test]
    fn test_stop_content_json_serialization() {
        let context = BehavioralMetricContext::from("test-app");

        let stop_content = StopContent {
            context,
            entity_id: Some("entity-456".to_string()),
            age_policy: Some(AgePolicy::Teen),
        };

        let json = serde_json::to_string(&stop_content).expect("Should serialize");
        let deserialized: StopContent = serde_json::from_str(&json).expect("Should deserialize");
        assert_eq!(deserialized, stop_content);
    }

    #[test]
    fn test_page_json_serialization() {
        let context = BehavioralMetricContext::from("test-app");

        let page = Page {
            context,
            page_id: "home".to_string(),
            age_policy: Some(AgePolicy::Adult),
        };

        let json = serde_json::to_string(&page).expect("Should serialize");
        assert!(json.contains("home"));

        let deserialized: Page = serde_json::from_str(&json).expect("Should deserialize");
        assert_eq!(deserialized, page);

        // Test Firebolt spec compliance
        let firebolt_page_json = r#"{
            "context": {
                "app_id": "test-app",
                "product_version": "product.version.not.implemented", 
                "partner_id": "partner.id.not.set",
                "app_session_id": "app_session_id.not.set",
                "durable_app_id": "test-app"
            },
            "page_id": "xyz"
        }"#;

        let from_firebolt: Page =
            serde_json::from_str(firebolt_page_json).expect("Should deserialize Firebolt example");
        assert_eq!(from_firebolt.page_id, "xyz");
    }

    #[test]
    fn test_action_json_serialization() {
        let context = BehavioralMetricContext::from("test-app");

        let action = Action {
            context,
            category: CategoryType::user,
            _type: "click".to_string(),
            parameters: vec![
                Param {
                    name: "button".to_string(),
                    value: FlatMapValue::String("submit".to_string()),
                },
                Param {
                    name: "count".to_string(),
                    value: FlatMapValue::Number(42.0),
                },
            ],
            age_policy: Some(AgePolicy::Child),
        };

        let json = serde_json::to_string(&action).expect("Should serialize");
        assert!(json.contains("click"));
        assert!(json.contains("user"));
        assert!(json.contains("button"));

        let deserialized: Action = serde_json::from_str(&json).expect("Should deserialize");
        assert_eq!(deserialized, action);

        // Test with app category
        let app_action = Action {
            context: BehavioralMetricContext::from("test-app"),
            category: CategoryType::app,
            _type: "The user did foo".to_string(),
            parameters: vec![],
            age_policy: None,
        };

        let app_json = serde_json::to_string(&app_action).expect("Should serialize app action");
        let app_deserialized: Action =
            serde_json::from_str(&app_json).expect("Should deserialize app action");
        assert_eq!(app_deserialized.category, CategoryType::app);

        // Test JSON field rename for "type"
        assert!(json.contains(r#""type":"click""#));
    }

    #[test]
    fn test_metrics_error_json_serialization() {
        let context = BehavioralMetricContext::from("test-app");

        let mut parameters = HashMap::new();
        parameters.insert(
            "error_detail".to_string(),
            FlatMapValue::String("Network timeout".to_string()),
        );

        let metrics_error = MetricsError {
            context,
            error_type: ErrorType::network,
            code: "NETWORK_TIMEOUT".to_string(),
            description: "Network connection timed out".to_string(),
            visible: true,
            parameters: Some(parameters),
            durable_app_id: "test-durable-app".to_string(),
            third_party_error: false,
            age_policy: Some(AgePolicy::Adult),
        };

        let json = serde_json::to_string(&metrics_error).expect("Should serialize");
        assert!(json.contains("NETWORK_TIMEOUT"));
        assert!(json.contains("Network connection timed out"));

        let deserialized: MetricsError = serde_json::from_str(&json).expect("Should deserialize");
        assert_eq!(deserialized, metrics_error);

        // Test minimal error
        let minimal_error = MetricsError {
            context: BehavioralMetricContext::from("test-app"),
            error_type: ErrorType::media,
            code: "MEDIA-STALLED".to_string(),
            description: "playback stalled".to_string(),
            visible: true,
            parameters: None,
            durable_app_id: "test-app".to_string(),
            third_party_error: false,
            age_policy: None,
        };

        let minimal_json = serde_json::to_string(&minimal_error).expect("Should serialize minimal");
        let minimal_deserialized: MetricsError =
            serde_json::from_str(&minimal_json).expect("Should deserialize minimal");
        assert_eq!(minimal_deserialized, minimal_error);
    }

    #[test]
    fn test_media_metrics_json_serialization() {
        let context = BehavioralMetricContext::from("test-app");

        // Test MediaLoadStart
        let media_load_start = MediaLoadStart {
            context: context.clone(),
            entity_id: "media-345".to_string(),
            age_policy: Some(AgePolicy::Teen),
        };

        let json =
            serde_json::to_string(&media_load_start).expect("Should serialize MediaLoadStart");
        assert!(json.contains("media-345"));
        let deserialized: MediaLoadStart =
            serde_json::from_str(&json).expect("Should deserialize MediaLoadStart");
        assert_eq!(deserialized, media_load_start);

        // Test MediaPlay
        let media_play = MediaPlay {
            context: context.clone(),
            entity_id: "media-345".to_string(),
            age_policy: None,
        };

        let play_json = serde_json::to_string(&media_play).expect("Should serialize MediaPlay");
        let play_deserialized: MediaPlay =
            serde_json::from_str(&play_json).expect("Should deserialize MediaPlay");
        assert_eq!(play_deserialized, media_play);

        // Test MediaProgress with percentage
        let media_progress = MediaProgress {
            context: context.clone(),
            entity_id: "media-345".to_string(),
            progress: Some(MediaPositionType::PercentageProgress(0.75)),
            age_policy: Some(AgePolicy::Adult),
        };

        let progress_json =
            serde_json::to_string(&media_progress).expect("Should serialize MediaProgress");
        let progress_deserialized: MediaProgress =
            serde_json::from_str(&progress_json).expect("Should deserialize MediaProgress");
        assert_eq!(progress_deserialized, media_progress);

        // Test MediaProgress with absolute position
        let absolute_progress = MediaProgress {
            context: context.clone(),
            entity_id: "media-345".to_string(),
            progress: Some(MediaPositionType::AbsolutePosition(1800)), // 30 minutes
            age_policy: None,
        };

        let abs_json =
            serde_json::to_string(&absolute_progress).expect("Should serialize absolute progress");
        let abs_deserialized: MediaProgress =
            serde_json::from_str(&abs_json).expect("Should deserialize absolute progress");
        assert_eq!(abs_deserialized, absolute_progress);
    }

    #[test]
    fn test_media_seeking_seeked_json_serialization() {
        let context = BehavioralMetricContext::from("test-app");

        // Test MediaSeeking
        let media_seeking = MediaSeeking {
            context: context.clone(),
            entity_id: "media-345".to_string(),
            target: Some(MediaPositionType::PercentageProgress(0.50)),
            age_policy: Some(AgePolicy::Child),
        };

        let seeking_json =
            serde_json::to_string(&media_seeking).expect("Should serialize MediaSeeking");
        let seeking_deserialized: MediaSeeking =
            serde_json::from_str(&seeking_json).expect("Should deserialize MediaSeeking");
        assert_eq!(seeking_deserialized, media_seeking);

        // Test MediaSeeked
        let media_seeked = MediaSeeked {
            context,
            entity_id: "media-345".to_string(),
            position: Some(MediaPositionType::PercentageProgress(0.51)),
            age_policy: None,
        };

        let seeked_json =
            serde_json::to_string(&media_seeked).expect("Should serialize MediaSeeked");
        let seeked_deserialized: MediaSeeked =
            serde_json::from_str(&seeked_json).expect("Should deserialize MediaSeeked");
        assert_eq!(seeked_deserialized, media_seeked);
    }

    #[test]
    fn test_media_rate_changed_json_serialization() {
        let context = BehavioralMetricContext::from("test-app");

        let media_rate_changed = MediaRateChanged {
            context,
            entity_id: "media-345".to_string(),
            rate: Number::from(2), // 2x speed
            age_policy: Some(AgePolicy::Teen),
        };

        let json =
            serde_json::to_string(&media_rate_changed).expect("Should serialize MediaRateChanged");
        assert!(json.contains("media-345"));

        let deserialized: MediaRateChanged =
            serde_json::from_str(&json).expect("Should deserialize MediaRateChanged");
        assert_eq!(deserialized, media_rate_changed);

        // Test fractional rate
        let fractional_rate = MediaRateChanged {
            context: BehavioralMetricContext::from("test-app"),
            entity_id: "media-345".to_string(),
            rate: serde_json::Number::from_f64(0.5).unwrap(), // 0.5x speed
            age_policy: None,
        };

        let frac_json =
            serde_json::to_string(&fractional_rate).expect("Should serialize fractional rate");
        let frac_deserialized: MediaRateChanged =
            serde_json::from_str(&frac_json).expect("Should deserialize fractional rate");
        assert_eq!(frac_deserialized, fractional_rate);
    }

    #[test]
    fn test_media_rendition_changed_json_serialization() {
        let context = BehavioralMetricContext::from("test-app");

        let media_rendition_changed = MediaRenditionChanged {
            context,
            entity_id: "media-345".to_string(),
            bitrate: Number::from(5000),
            width: 1920,
            height: 1080,
            profile: Some("HDR+".to_string()),
            age_policy: Some(AgePolicy::Adult),
        };

        let json = serde_json::to_string(&media_rendition_changed)
            .expect("Should serialize MediaRenditionChanged");
        assert!(json.contains("5000"));
        assert!(json.contains("1920"));
        assert!(json.contains("1080"));
        assert!(json.contains("HDR+"));

        let deserialized: MediaRenditionChanged =
            serde_json::from_str(&json).expect("Should deserialize MediaRenditionChanged");
        assert_eq!(deserialized, media_rendition_changed);

        // Test without optional profile
        let no_profile = MediaRenditionChanged {
            context: BehavioralMetricContext::from("test-app"),
            entity_id: "media-345".to_string(),
            bitrate: Number::from(2500),
            width: 1280,
            height: 720,
            profile: None,
            age_policy: None,
        };

        let no_profile_json =
            serde_json::to_string(&no_profile).expect("Should serialize without profile");
        let no_profile_deserialized: MediaRenditionChanged =
            serde_json::from_str(&no_profile_json).expect("Should deserialize without profile");
        assert_eq!(no_profile_deserialized, no_profile);
    }

    #[test]
    fn test_media_ended_json_serialization() {
        let context = BehavioralMetricContext::from("test-app");

        let media_ended = MediaEnded {
            context,
            entity_id: "media-345".to_string(),
            age_policy: Some(AgePolicy::Child),
        };

        let json = serde_json::to_string(&media_ended).expect("Should serialize MediaEnded");
        let deserialized: MediaEnded =
            serde_json::from_str(&json).expect("Should deserialize MediaEnded");
        assert_eq!(deserialized, media_ended);
    }

    #[test]
    fn test_media_position_type_json_serialization() {
        // Test None
        let none_position = MediaPositionType::None;
        let none_json = serde_json::to_string(&none_position).expect("Should serialize None");
        let none_deserialized: MediaPositionType =
            serde_json::from_str(&none_json).expect("Should deserialize None");
        assert_eq!(none_deserialized, none_position);

        // Test PercentageProgress
        let percentage = MediaPositionType::PercentageProgress(0.75);
        let percentage_json =
            serde_json::to_string(&percentage).expect("Should serialize percentage");
        let percentage_deserialized: MediaPositionType =
            serde_json::from_str(&percentage_json).expect("Should deserialize percentage");
        assert_eq!(percentage_deserialized, percentage);

        // Test AbsolutePosition
        let absolute = MediaPositionType::AbsolutePosition(1800);
        let absolute_json = serde_json::to_string(&absolute).expect("Should serialize absolute");
        let absolute_deserialized: MediaPositionType =
            serde_json::from_str(&absolute_json).expect("Should deserialize absolute");
        assert_eq!(absolute_deserialized, absolute);
    }

    #[test]
    fn test_raw_behavior_metric_request_json_serialization() {
        let context = BehavioralMetricContext::from("test-app");
        let value = serde_json::json!({
            "custom_field": "custom_value",
            "numeric_field": 42,
            "boolean_field": true
        });

        let raw_request = RawBehaviorMetricRequest { context, value };

        let json =
            serde_json::to_string(&raw_request).expect("Should serialize RawBehaviorMetricRequest");
        assert!(json.contains("custom_field"));
        assert!(json.contains("custom_value"));

        let deserialized: RawBehaviorMetricRequest =
            serde_json::from_str(&json).expect("Should deserialize RawBehaviorMetricRequest");
        assert_eq!(deserialized, raw_request);
    }

    #[test]
    fn test_behavioral_metric_payload_enum_json_serialization() {
        let context = BehavioralMetricContext::from("test-app");

        // Test Ready variant
        let ready_payload = BehavioralMetricPayload::Ready(Ready {
            context: context.clone(),
            ttmu_ms: 1000,
        });

        let ready_json =
            serde_json::to_string(&ready_payload).expect("Should serialize Ready payload");
        let ready_deserialized: BehavioralMetricPayload =
            serde_json::from_str(&ready_json).expect("Should deserialize Ready payload");
        if let BehavioralMetricPayload::Ready(ready) = ready_deserialized {
            assert_eq!(ready.ttmu_ms, 1000);
        } else {
            panic!("Expected Ready variant");
        }

        // Test StartContent variant
        let start_content_payload = BehavioralMetricPayload::StartContent(StartContent {
            context: context.clone(),
            entity_id: Some("entity-123".to_string()),
            age_policy: Some(AgePolicy::Child),
        });

        let start_json = serde_json::to_string(&start_content_payload)
            .expect("Should serialize StartContent payload");
        let start_deserialized: BehavioralMetricPayload =
            serde_json::from_str(&start_json).expect("Should deserialize StartContent payload");
        if let BehavioralMetricPayload::StartContent(start_content) = start_deserialized {
            assert_eq!(start_content.entity_id, Some("entity-123".to_string()));
        } else {
            panic!("Expected StartContent variant");
        }

        // Test Action variant
        let action_payload = BehavioralMetricPayload::Action(Action {
            context,
            category: CategoryType::user,
            _type: "button_click".to_string(),
            parameters: vec![],
            age_policy: None,
        });

        let action_json =
            serde_json::to_string(&action_payload).expect("Should serialize Action payload");
        let action_deserialized: BehavioralMetricPayload =
            serde_json::from_str(&action_json).expect("Should deserialize Action payload");
        if let BehavioralMetricPayload::Action(action) = action_deserialized {
            assert_eq!(action._type, "button_click");
        } else {
            panic!("Expected Action variant");
        }
    }

    #[test]
    fn test_sift_app_lifecycle_state_change_json_serialization() {
        let context = BehavioralMetricContext::from("test-app");

        let state_change = SiftAppLifecycleStateChange {
            context,
            previous_state: Some(AppLifecycleState::Foreground),
            new_state: AppLifecycleState::Background,
        };

        let json = serde_json::to_string(&state_change)
            .expect("Should serialize SiftAppLifecycleStateChange");
        let deserialized: SiftAppLifecycleStateChange =
            serde_json::from_str(&json).expect("Should deserialize SiftAppLifecycleStateChange");
        assert_eq!(deserialized, state_change);

        // Test with no previous state
        let initial_state_change = SiftAppLifecycleStateChange {
            context: BehavioralMetricContext::from("test-app"),
            previous_state: None,
            new_state: AppLifecycleState::Foreground,
        };

        let initial_json = serde_json::to_string(&initial_state_change)
            .expect("Should serialize initial state change");
        let initial_deserialized: SiftAppLifecycleStateChange =
            serde_json::from_str(&initial_json).expect("Should deserialize initial state change");
        assert_eq!(initial_deserialized, initial_state_change);
    }

    #[test]
    fn test_metrics_context_json_serialization() {
        let mut metrics_context = MetricsContext::new();
        metrics_context.enabled = true;
        metrics_context.device_language = "en-US".to_string();
        metrics_context.device_model = "TestDevice".to_string();
        metrics_context.device_id = Some("device-123".to_string());
        metrics_context.account_id = Some("account-456".to_string());
        metrics_context.age_policy = Some(AgePolicy::Teen);
        metrics_context.age_policy_cets = Some(vec!["PG".to_string(), "TV-14".to_string()]);
        metrics_context.authenticated = Some(true);

        let json =
            serde_json::to_string(&metrics_context).expect("Should serialize MetricsContext");
        assert!(json.contains("en-US"));
        assert!(json.contains("TestDevice"));
        assert!(json.contains("device-123"));

        let deserialized: MetricsContext =
            serde_json::from_str(&json).expect("Should deserialize MetricsContext");
        assert_eq!(deserialized, metrics_context);

        // Test minimal MetricsContext
        let minimal_context = MetricsContext::new();
        let minimal_json =
            serde_json::to_string(&minimal_context).expect("Should serialize minimal context");
        let minimal_deserialized: MetricsContext =
            serde_json::from_str(&minimal_json).expect("Should deserialize minimal context");
        assert_eq!(minimal_deserialized, minimal_context);
    }

    #[test]
    fn test_app_data_governance_state_json_serialization() {
        let governance_state = AppDataGovernanceState {
            data_tags_to_apply: [
                "child".to_string(),
                "family".to_string(),
                "educational".to_string(),
            ]
            .into_iter()
            .collect(),
        };

        let json = serde_json::to_string(&governance_state)
            .expect("Should serialize AppDataGovernanceState");
        assert!(json.contains("child"));
        assert!(json.contains("family"));
        assert!(json.contains("educational"));

        let deserialized: AppDataGovernanceState =
            serde_json::from_str(&json).expect("Should deserialize AppDataGovernanceState");
        assert_eq!(deserialized, governance_state);

        // Test empty governance state
        let empty_state = AppDataGovernanceState::default();
        let empty_json =
            serde_json::to_string(&empty_state).expect("Should serialize empty governance state");
        let empty_deserialized: AppDataGovernanceState =
            serde_json::from_str(&empty_json).expect("Should deserialize empty governance state");
        assert_eq!(empty_deserialized, empty_state);
    }

    #[test]
    fn test_badger_metrics_json_serialization() {
        let context = BehavioralMetricContext::from("test-badger-app");

        // Test BadgerMetric
        let badger_metric = BadgerMetric {
            context: context.clone(),
            segment: Some("video".to_string()),
            args: Some(vec![Param {
                name: "duration".to_string(),
                value: FlatMapValue::Number(120.0),
            }]),
        };

        let metric_json =
            serde_json::to_string(&badger_metric).expect("Should serialize BadgerMetric");
        assert!(metric_json.contains("video"));
        // Note: context is skipped in serialization, so we manually construct JSON for deserialization test
        let metric_json_with_context = format!(
            r#"{{"context":{{"app_id":"test-badger-app","product_version":"product.version.not.implemented","partner_id":"partner.id.not.set","app_session_id":"app_session_id.not.set","durable_app_id":"test-badger-app"}},"segment":"video","args":[{{"name":"duration","value":120.0}}]}}"#
        );
        let metric_deserialized: BadgerMetric = serde_json::from_str(&metric_json_with_context)
            .expect("Should deserialize BadgerMetric");
        assert_eq!(metric_deserialized.segment, Some("video".to_string()));

        // Test BadgerError
        let badger_error = BadgerError {
            context: context.clone(),
            message: "Connection failed".to_string(),
            visible: true,
            code: 500,
            args: None,
        };

        let _error_json =
            serde_json::to_string(&badger_error).expect("Should serialize BadgerError");
        let error_json_with_context = format!(
            r#"{{"context":{{"app_id":"test-badger-app","product_version":"product.version.not.implemented","partner_id":"partner.id.not.set","app_session_id":"app_session_id.not.set","durable_app_id":"test-badger-app"}},"message":"Connection failed","visible":true,"code":500}}"#
        );
        let error_deserialized: BadgerError =
            serde_json::from_str(&error_json_with_context).expect("Should deserialize BadgerError");
        assert_eq!(error_deserialized.message, badger_error.message);
        assert_eq!(error_deserialized.code, badger_error.code);

        // Test BadgerPageView
        let page_view = BadgerPageView {
            context,
            page: "settings".to_string(),
            args: Some(vec![Param {
                name: "section".to_string(),
                value: FlatMapValue::String("audio".to_string()),
            }]),
        };

        let _page_json =
            serde_json::to_string(&page_view).expect("Should serialize BadgerPageView");
        let page_json_with_context = format!(
            r#"{{"context":{{"app_id":"test-badger-app","product_version":"product.version.not.implemented","partner_id":"partner.id.not.set","app_session_id":"app_session_id.not.set","durable_app_id":"test-badger-app"}},"page":"settings","args":[{{"name":"section","value":"audio"}}]}}"#
        );
        let page_deserialized: BadgerPageView = serde_json::from_str(&page_json_with_context)
            .expect("Should deserialize BadgerPageView");
        assert_eq!(page_deserialized.page, page_view.page);
    }

    #[test]
    fn test_badger_metrics_enum_json_serialization() {
        let context = BehavioralMetricContext::from("test-badger-app");

        // Test Metric variant
        let metric_variant = BadgerMetrics::Metric(BadgerMetric {
            context: context.clone(),
            segment: Some("analytics".to_string()),
            args: None,
        });

        let _metric_json =
            serde_json::to_string(&metric_variant).expect("Should serialize Metric variant");
        // Since context is skipped in serialization for Badger structs, we manually create JSON for deserialization
        let metric_json_with_context = format!(
            r#"{{"Metric":{{"context":{{"app_id":"test-badger-app","product_version":"product.version.not.implemented","partner_id":"partner.id.not.set","app_session_id":"app_session_id.not.set","durable_app_id":"test-badger-app"}},"segment":"analytics"}}}}"#
        );
        let metric_deserialized: BadgerMetrics = serde_json::from_str(&metric_json_with_context)
            .expect("Should deserialize Metric variant");
        if let BadgerMetrics::Metric(metric) = metric_deserialized {
            assert_eq!(metric.segment, Some("analytics".to_string()));
        } else {
            panic!("Expected Metric variant");
        }

        // Test LaunchCompleted variant
        let launch_variant = BadgerMetrics::LaunchCompleted(BadgerLaunchCompleted {
            context,
            args: Some(vec![Param {
                name: "launch_time".to_string(),
                value: FlatMapValue::Number(2500.0),
            }]),
        });

        let _launch_json = serde_json::to_string(&launch_variant)
            .expect("Should serialize LaunchCompleted variant");
        let launch_json_with_context = format!(
            r#"{{"LaunchCompleted":{{"context":{{"app_id":"test-badger-app","product_version":"product.version.not.implemented","partner_id":"partner.id.not.set","app_session_id":"app_session_id.not.set","durable_app_id":"test-badger-app"}},"args":[{{"name":"launch_time","value":2500.0}}]}}}}"#
        );
        let launch_deserialized: BadgerMetrics = serde_json::from_str(&launch_json_with_context)
            .expect("Should deserialize LaunchCompleted variant");
        if let BadgerMetrics::LaunchCompleted(launch) = launch_deserialized {
            assert!(launch.args.is_some());
        } else {
            panic!("Expected LaunchCompleted variant");
        }
    }

    // Negative test cases for JSON schema compliance

    #[test]
    fn test_json_deserialization_error_cases() {
        // Test missing required fields
        let invalid_ready_json = r#"{"context": {}}"#; // missing ttmu_ms
        assert!(serde_json::from_str::<Ready>(invalid_ready_json).is_err());

        let invalid_page_json = r#"{"context": {}}"#; // missing page_id
        assert!(serde_json::from_str::<Page>(invalid_page_json).is_err());

        let invalid_action_json = r#"{"context": {}, "category": "user"}"#; // missing type
        assert!(serde_json::from_str::<Action>(invalid_action_json).is_err());

        // Test invalid enum values
        let invalid_category_json =
            r#"{"context": {}, "category": "invalid", "type": "test", "parameters": []}"#;
        assert!(serde_json::from_str::<Action>(invalid_category_json).is_err());

        let invalid_error_type_json = r#"{"context": {}, "type": "invalid_error_type", "code": "TEST", "description": "test", "visible": true, "durable_app_id": "test", "third_party_error": false}"#;
        assert!(serde_json::from_str::<MetricsError>(invalid_error_type_json).is_err());

        // Test invalid data types
        let invalid_ttmu_json = r#"{"context": {}, "ttmu_ms": "not_a_number"}"#;
        assert!(serde_json::from_str::<Ready>(invalid_ttmu_json).is_err());

        let invalid_visible_json = r#"{"context": {}, "type": "media", "code": "TEST", "description": "test", "visible": "not_boolean", "durable_app_id": "test", "third_party_error": false}"#;
        assert!(serde_json::from_str::<MetricsError>(invalid_visible_json).is_err());
    }

    #[test]
    fn test_firebolt_openrpc_schema_compliance() {
        // Test examples that should match the Firebolt OpenRPC specification

        // startContent with entityId example
        let start_content_json = r#"{
            "context": {
                "app_id": "test-app",
                "product_version": "1.0.0",
                "partner_id": "test-partner",
                "app_session_id": "session-123",
                "durable_app_id": "test-app"
            },
            "entity_id": "abc"
        }"#;

        let start_content: StartContent = serde_json::from_str(start_content_json)
            .expect("Should parse Firebolt startContent example");
        assert_eq!(start_content.entity_id, Some("abc".to_string()));

        // page example
        let page_json = r#"{
            "context": {
                "app_id": "test-app",
                "product_version": "1.0.0",
                "partner_id": "test-partner", 
                "app_session_id": "session-123",
                "durable_app_id": "test-app"
            },
            "page_id": "xyz"
        }"#;

        let page: Page =
            serde_json::from_str(page_json).expect("Should parse Firebolt page example");
        assert_eq!(page.page_id, "xyz");

        // action example
        let action_json = r#"{
            "context": {
                "app_id": "test-app",
                "product_version": "1.0.0",
                "partner_id": "test-partner",
                "app_session_id": "session-123", 
                "durable_app_id": "test-app"
            },
            "category": "user",
            "type": "The user did foo",
            "parameters": []
        }"#;

        let action: Action =
            serde_json::from_str(action_json).expect("Should parse Firebolt action example");
        assert_eq!(action.category, CategoryType::user);
        assert_eq!(action._type, "The user did foo");

        // error example
        let error_json = r#"{
            "context": {
                "app_id": "test-app",
                "product_version": "1.0.0",
                "partner_id": "test-partner",
                "app_session_id": "session-123",
                "durable_app_id": "test-app"
            },
            "type": "media",
            "code": "MEDIA-STALLED",
            "description": "playback stalled",
            "visible": true,
            "durable_app_id": "test-app",
            "third_party_error": false
        }"#;

        let error: MetricsError =
            serde_json::from_str(error_json).expect("Should parse Firebolt error example");
        assert_eq!(error.error_type, ErrorType::media);
        assert_eq!(error.code, "MEDIA-STALLED");

        // mediaProgress example
        let progress_json = r#"{
            "context": {
                "app_id": "test-app",
                "product_version": "1.0.0",
                "partner_id": "test-partner",
                "app_session_id": "session-123",
                "durable_app_id": "test-app"
            },
            "entity_id": "345",
            "progress": {"PercentageProgress": 0.75}
        }"#;

        let progress: MediaProgress = serde_json::from_str(progress_json)
            .expect("Should parse Firebolt mediaProgress example");
        assert_eq!(progress.entity_id, "345");
        if let Some(MediaPositionType::PercentageProgress(pct)) = progress.progress {
            assert!((pct - 0.75).abs() < f32::EPSILON);
        } else {
            panic!("Expected percentage progress");
        }
    }
}
