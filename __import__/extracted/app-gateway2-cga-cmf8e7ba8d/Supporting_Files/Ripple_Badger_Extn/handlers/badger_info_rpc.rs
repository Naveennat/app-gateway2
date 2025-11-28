// Copyright 2023 Comcast Cable Communications Management, LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
//

use crate::badger_ffi::BADGER_SERVICE_ID;
use crate::{
    authorizer::Authorizer, badger_state::BadgerState, crypto_util::salt_using_app_scope,
    handlers::badger_device_info::get_device_capabilities,
    thunder::thunder_session::ThunderSessionService,
};
use jsonrpsee::{core::Error::Custom as CustomErr, core::RpcResult, proc_macros::rpc};
use ripple_sdk::api::firebolt::fb_keyboard::KeyboardSessionResponse;
use ripple_sdk::api::firebolt::fb_pin::PinChallengeResponse;
use ripple_sdk::{
    api::{
        apps::{AppEvent, AppEventRequest, CloseReason},
        caps::CapsRequest,
        device::{
            device_info_request::{
                DEVICE_INFO_AUTHORIZED, DEVICE_MAKE_MODEL_AUTHORIZED, DEVICE_SKU_AUTHORIZED,
            },
            device_request::{HdrProfile, NetworkResponse, NetworkType},
            entertainment_data::{
                NavigationIntent, NavigationIntentStrict, SectionIntent, SectionIntentData,
            },
        },
        firebolt::{
            fb_discovery::{DiscoveryContext, NavigateCompanyPageRequest},
            fb_general::{ListenRequest, ListenerResponse},
            fb_keyboard::{KeyboardSessionRequest, KeyboardType},
            fb_lifecycle::CloseRequest,
            fb_parameters::AppInitParameters,
            fb_pin::{PinChallengeRequestWithContext, PinChallengeResultReason, PinSpace},
            fb_secondscreen::SECOND_SCREEN_EVENT_ON_LAUNCH_REQUEST,
            provider::ChallengeRequestor,
        },
        gateway::rpc_gateway_api::CallContext,
        session::AccountSession,
        settings::{SettingKey, SettingValue, SettingsRequest, SettingsRequestParam},
    },
    async_trait::async_trait,
    extn::client::extn_client::ExtnClient,
    log::{debug, error},
    serde_json,
    utils::rpc_utils::rpc_error_with_code,
    JsonRpcErrorType,
};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

pub struct BadgerServiceImpl {
    state: BadgerState,
}
const DOWNSTREAM_SERVICE_UNAVAILABLE_ERROR_CODE: i32 = -50200;

enum UidType {
    Account,
    Device,
}

pub fn rpc_downstream_service_err(msg: &str) -> JsonRpcErrorType {
    rpc_error_with_code::<String>(msg, DOWNSTREAM_SERVICE_UNAVAILABLE_ERROR_CODE)
}

#[derive(Default, Serialize, Debug)]
#[cfg_attr(test, derive(PartialEq))]
pub struct BadgerEmptyResult {
    //Empty object to take care of OTTX-28709
}

#[derive(Serialize, Clone, Debug, Deserialize)]
#[cfg_attr(test, derive(PartialEq))]
#[serde(rename_all = "camelCase")]
pub struct BadgerHDR {
    settop_hdr_support: Vec<BadgerHdrProfile>,
    tv_hdr_support: Vec<BadgerHdrProfile>,
}

#[derive(Hash, Eq, PartialEq, Debug, Serialize, Deserialize, Clone, Copy)]
pub enum BadgerHdrProfile {
    #[serde(rename = "HDR10")]
    Hdr10,
    #[serde(rename = "HDR10+")]
    Hdr10plus,
    #[serde(rename = "HLG")]
    Hlg,
    #[serde(rename = "Dolby Vision")]
    DolbyVision,
    #[serde(rename = "Technicolor")]
    Technicolor,
}

impl From<HdrProfile> for BadgerHdrProfile {
    fn from(value: HdrProfile) -> Self {
        match value {
            HdrProfile::DolbyVision => BadgerHdrProfile::DolbyVision,
            HdrProfile::Hlg => BadgerHdrProfile::Hlg,
            HdrProfile::Hdr10 => BadgerHdrProfile::Hdr10,
            HdrProfile::Hdr10plus => BadgerHdrProfile::Hdr10plus,
            HdrProfile::Technicolor => BadgerHdrProfile::Technicolor,
        }
    }
}

#[derive(Serialize, Clone, Debug, Deserialize)]
#[cfg_attr(test, derive(PartialEq))]
#[serde(rename_all = "camelCase")]
pub struct BadgerWebBrowser {
    pub(crate) user_agent: String,
    pub(crate) version: String,
    pub(crate) browser_type: String,
}

#[derive(Serialize, Clone, Debug, Deserialize, Default)]
#[cfg_attr(test, derive(PartialEq))]
pub struct BadgerHDCP {
    #[serde(rename = "supportedHDCPVersion")]
    supported_hdcp_version: String,
    #[serde(rename = "receiverHDCPVersion")]
    receiver_hdcp_version: String,
    #[serde(rename = "currentHDCPVersion")]
    current_hdcp_version: String,
    connected: bool,
    #[serde(rename = "hdcpcompliant")]
    hdcp_compliant: bool,
    #[serde(rename = "hdcpenabled")]
    hdcp_enabled: bool,
    #[serde(rename = "hdcpReason")]
    hdcp_reason: u32,
}

#[derive(Serialize, Debug, Deserialize)]
#[cfg_attr(test, derive(PartialEq))]
#[serde(rename_all = "camelCase")]
pub struct BadgerAudioModes {
    current_audio_mode: Vec<String>,
    supported_audio_modes: Vec<String>,
}

#[derive(Serialize, Deserialize, Debug, Default)]
#[cfg_attr(test, derive(PartialEq))]
#[serde(rename_all = "camelCase")]
pub struct BadgerDeviceCapabilities {
    #[serde(skip_serializing_if = "Option::is_none")]
    device_type: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    is_wifi_device: Option<bool>,
    #[serde(skip_serializing_if = "Option::is_none")]
    video_dimensions: Option<Vec<i32>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    native_dimensions: Option<Vec<i32>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    model: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    receiver_platform: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    receiver_version: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    hdr: Option<BadgerHDR>,
    #[serde(skip_serializing_if = "Option::is_none")]
    hdcp: Option<BadgerHDCP>,
    #[serde(skip_serializing_if = "Option::is_none")]
    web_browser: Option<BadgerWebBrowser>,
    #[serde(rename = "supportsTrueSD", skip_serializing_if = "Option::is_none")]
    supports_true_sd: Option<bool>,
    #[serde(skip_serializing_if = "Option::is_none")]
    device_make_model: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    audio_modes: Option<BadgerAudioModes>,
}
#[derive(Serialize, Clone, Debug)]
#[cfg_attr(test, derive(PartialEq))]
#[serde(rename_all = "camelCase")]
pub struct BadgerNetworkConnectivity {
    network_interface: Option<String>,
    status: BadgerNetworkConnectivityStatus,
}

#[derive(Serialize, Clone, Debug, PartialEq)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
pub enum BadgerNetworkConnectivityStatus {
    NoActiveNetworkInterface,
    Success,
}

#[derive(Serialize, Debug, Deserialize, Default)]
#[cfg_attr(test, derive(PartialEq))]
#[serde(rename_all = "camelCase")]
pub struct BadgerDeviceInfo {
    #[serde(rename = "zipcode", skip_serializing_if = "Option::is_none")]
    pub(crate) zip_code: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    time_zone: Option<String>,
    #[serde(rename = "timezone", skip_serializing_if = "Option::is_none")]
    time_zone_offset: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    receiver_id: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    device_hash: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    device_id: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    account_id: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    device_capabilities: Option<BadgerDeviceCapabilities>,
    #[serde(skip_serializing_if = "Option::is_none")]
    household_id: Option<String>,
    privacy_settings: HashMap<String, String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    partner_id: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    user_experience: Option<String>,
}

#[derive(Serialize, Clone, Debug, Deserialize, Default)]
#[cfg_attr(test, derive(PartialEq))]
#[serde(rename_all = "camelCase")]
pub struct DefaultResponse {}

async fn get_cached_is_wifi_device(state: &BadgerState) -> Option<bool> {
    match state.get_is_wifi_device() {
        Some(is_wifi_device) => Some(is_wifi_device),
        None => get_is_wifi_device(state).await,
    }
}

pub(crate) async fn get_is_wifi_device(state: &BadgerState) -> Option<bool> {
    let wifi_caps = vec!["xrn:firebolt:capability:protocol:wifi".to_string()];
    let supported = CapsRequest::Supported(wifi_caps);
    let params = serde_json::to_value(supported).unwrap();

    let result: Result<HashMap<String, bool>, _> = state
        .get_service_client()
        .call_and_parse_ripple_main_rpc(
            "ripple.checkCapsRequest",
            Some(params),
            None,
            5000,
            BADGER_SERVICE_ID,
            "Failed to get wifi capabilities",
        )
        .await;

    match result {
        Ok(supported_caps) => {
            debug!("Supported capabilities: {:?}", supported_caps);
            let is_wifi = supported_caps.values().any(|&value| value);
            state.update_is_wifi_device(Some(is_wifi));
            Some(is_wifi)
        }
        Err(e) => {
            error!("Failed to get wifi capabilities: {:?}", e);
            state.update_is_wifi_device(None);
            None
        }
    }
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct DistributorTokenResponse {
    pub access_token: String,
    pub token_type: String,
    pub scope: String,
    pub expires_in: i32,
    pub tid: String,
}

#[derive(Serialize, Clone, Debug)]
pub struct PromptEmailResult {
    status: PromptEmailStatus,
    data: PromptEmailData,
}

#[derive(Deserialize, Clone, Debug)]
#[serde(rename_all = "camelCase")]
pub struct PromptEmailRequest {
    prefill_type: PrefillType,
}

#[derive(Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub enum PrefillType {
    SignIn,
    SignUp,
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum EmailUsage {
    SignIn,
    SignUp,
}

#[derive(Serialize, Debug, Clone)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
pub enum PromptEmailStatus {
    Success,
    Dismiss,
}

#[derive(Serialize, Debug, Clone)]
pub struct PromptEmailData {
    email: String,
}
#[derive(Deserialize, Debug, Clone)]
pub struct ShowPinOverlayRequest {
    pin_type: String,
    pub suppress_snooze: Option<SnoozeOption>,
}

#[derive(Deserialize, Serialize, Debug, Clone, PartialEq)]
pub struct BadgerPinOverlayResponse {
    status: BadgerPinStatus,
    message: String,
}

#[derive(Deserialize, Serialize, Debug, Clone, PartialEq)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
pub enum BadgerPinStatus {
    Success,
    PinNotRequired,
    Locked,
    UserDismissedOverlay,
}

#[derive(Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub enum SnoozeOption {
    True,
    False,
}

impl BadgerPinStatus {
    pub fn is_error(&self) -> bool {
        match self {
            BadgerPinStatus::Success => false,
            BadgerPinStatus::PinNotRequired => false,
            BadgerPinStatus::Locked => true,
            BadgerPinStatus::UserDismissedOverlay => true,
        }
    }

    pub fn to_string(&self, pin_type: &String) -> String {
        match self {
            BadgerPinStatus::Success => String::from("Successful Pin Validation"),
            BadgerPinStatus::PinNotRequired => {
                format!("Pin not required: {} not enabled", pin_type)
            }
            BadgerPinStatus::Locked => {
                String::from("Maximum attempts exceeded. Wait minute(s) before retrying")
            }
            BadgerPinStatus::UserDismissedOverlay => String::from("User dismissed Pin overlay"),
        }
    }

    pub fn from_result_reason(reason: PinChallengeResultReason) -> BadgerPinStatus {
        match reason {
            PinChallengeResultReason::NoPinRequired => BadgerPinStatus::PinNotRequired,
            PinChallengeResultReason::NoPinRequiredWindow => BadgerPinStatus::PinNotRequired,
            PinChallengeResultReason::ExceededPinFailures => BadgerPinStatus::Locked,
            PinChallengeResultReason::CorrectPin => BadgerPinStatus::Success,
            PinChallengeResultReason::Cancelled => BadgerPinStatus::UserDismissedOverlay,
        }
    }
}

fn from_badger_pin_type(pin_type: &str) -> PinSpace {
    match pin_type {
        "purchase_pin" => PinSpace::Purchase,
        _ => PinSpace::Content,
    }
}

#[derive(Serialize, Deserialize, Debug)]
#[serde(rename_all = "camelCase")]
pub struct BadgerDialDeviceId {
    pub device_id: String,
}

#[derive(Serialize, Deserialize, Debug)]
#[serde(rename_all = "camelCase")]
pub struct BadgerDialDeviceName {
    pub device_name: String,
}

#[derive(Serialize, Deserialize, Debug, Default, PartialEq)]
#[serde(rename_all = "camelCase")]
pub struct BadgerDialPayload {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub payload: Option<String>,
}

#[derive(Serialize, Deserialize, Debug)]
#[serde(rename_all = "camelCase")]
pub struct BadgerLaunchCallback {
    pub on_launch_callback: bool,
}

impl From<BadgerLaunchCallback> for ListenRequest {
    fn from(badger_launch_callback: BadgerLaunchCallback) -> Self {
        ListenRequest {
            listen: badger_launch_callback.on_launch_callback,
        }
    }
}

pub mod settings_serde {

    use serde::{Deserialize, Deserializer, Serializer};

    use super::from_key;

    pub fn serialize<S>(data: &[String], serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.collect_seq(data)
    }

    pub fn deserialize<'de, D>(deserializer: D) -> Result<Vec<String>, D::Error>
    where
        D: Deserializer<'de>,
    {
        let keys: Vec<String> = Vec::deserialize(deserializer)?;
        for key in &keys {
            if from_key(key).is_none() {
                return Err(serde::de::Error::custom(format!("Invalid key {}", key)));
            }
        }
        Ok(keys)
    }
}

#[derive(Clone, Debug, Deserialize)]
pub struct BadgerSettingsRequest {
    #[serde(with = "settings_serde")]
    pub keys: Vec<String>,
}

pub fn from_key(key: &str) -> Option<SettingKey> {
    match key {
        "VOICE_GUIDANCE_STATE" => Some(SettingKey::VoiceGuidanceEnabled),
        "TextToSpeechEnabled2" => Some(SettingKey::VoiceGuidanceEnabled),
        "CC_STATE" => Some(SettingKey::ClosedCaptions),
        "ShowClosedCapture" => Some(SettingKey::ClosedCaptions),
        "DisplayPersonalizedRecommendations" => Some(SettingKey::AllowPersonalization),
        "RememberWatchedPrograms" => Some(SettingKey::AllowWatchHistory),
        "ShareWatchHistoryStatus" => Some(SettingKey::ShareWatchHistory),
        "friendly_name" => Some(SettingKey::DeviceName),
        "power_save_status" => Some(SettingKey::PowerSaving),
        "legacyMiniGuide" => Some(SettingKey::LegacyMiniGuide),
        _ => None,
    }
}

#[rpc(server)]
pub trait BadgerService {
    #[method(name = "badger.info")]
    async fn badger_info(&self, ctx: CallContext) -> RpcResult<BadgerDeviceInfo>;

    #[method(name = "legacy.device.uid")]
    async fn legacy_device_uid(&self, ctx: CallContext) -> RpcResult<String>;

    #[method(name = "legacy.account.uid")]
    async fn legacy_account_uid(&self, ctx: CallContext) -> RpcResult<String>;

    #[method(name = "legacy.localization.postalCode")]
    async fn legacy_localization_postal_code(&self, ctx: CallContext) -> RpcResult<String>;

    #[method(name = "badger.deviceCapabilities")]
    async fn device_capabilities(&self, ctx: CallContext) -> RpcResult<BadgerDeviceCapabilities>;

    #[method(name = "badger.networkConnectivity")]
    async fn badger_network_connectivity(
        &self,
        ctx: CallContext,
    ) -> RpcResult<BadgerNetworkConnectivity>;

    #[method(name = "badger.shutdown")]
    async fn badger_shutdown(&self, ctx: CallContext) -> RpcResult<BadgerEmptyResult>;

    #[method(name = "badger.dial.getDeviceId", aliases = ["badger.getDeviceId"])]
    async fn badger_get_device_id(&self, ctx: CallContext) -> RpcResult<BadgerDialDeviceId>;

    #[method(name = "badger.dial.getDeviceName", aliases = ["badger.getDeviceName"])]
    async fn badger_get_device_name(&self, ctx: CallContext) -> RpcResult<BadgerDialDeviceName>;

    #[method(name = "badger.showToaster")]
    async fn show_toaster(&self, ctx: CallContext) -> RpcResult<DefaultResponse>;

    #[method(name = "badger.dismissLoadingScreen")]
    async fn badger_dismiss_loading_screen(&self, ctx: CallContext)
        -> RpcResult<BadgerEmptyResult>;

    #[method(name = "badger.dial.getPayload", aliases = ["badger.getPayload"])]
    async fn badger_get_payload(&self, ctx: CallContext) -> RpcResult<BadgerDialPayload>;

    #[method(name = "badger.dial.onLaunch", aliases = ["badger.onLaunch"])]
    async fn badger_on_launch(
        &self,
        ctx: CallContext,
        request: BadgerLaunchCallback,
    ) -> RpcResult<ListenerResponse>;

    #[method(name = "badger.navigateToCompanyPage")]
    async fn badger_navigate_to_company_page(
        &self,
        ctx: CallContext,
        req: NavigateCompanyPageRequest,
    ) -> RpcResult<BadgerEmptyResult>;

    #[method(name = "badger.promptEmail")]
    async fn prompt_email(
        &self,
        ctx: CallContext,
        request: PromptEmailRequest,
    ) -> RpcResult<PromptEmailResult>;

    #[method(name = "badger.showPinOverlay")]
    async fn badger_show_pin_overlay(
        &self,
        ctx: CallContext,
        request: ShowPinOverlayRequest,
    ) -> RpcResult<BadgerPinOverlayResponse>;

    #[method(name = "badger.settings")]
    async fn settings(
        &self,
        ctx: CallContext,
        req: BadgerSettingsRequest,
    ) -> RpcResult<HashMap<String, SettingValue>>;

    #[method(name = "badger.subscribeToSettings")]
    async fn subscribe_to_settings(
        &self,
        ctx: CallContext,
        req: BadgerSettingsRequest,
    ) -> RpcResult<()>;
}

impl BadgerServiceImpl {
    pub(crate) fn new(state: &BadgerState) -> BadgerServiceImpl {
        BadgerServiceImpl {
            state: state.clone(),
        }
    }

    async fn get_device_caps(
        &self,
        authorizer_info: &HashMap<String, bool>,
    ) -> RpcResult<BadgerDeviceCapabilities> {
        let state = self.state.clone();
        if Authorizer::check_device_info_required(authorizer_info) {
            let mut caps = BadgerDeviceCapabilities::default();
            let mut keys = vec![];
            if Authorizer::check_device_info_required(authorizer_info) {
                keys.push(DEVICE_INFO_AUTHORIZED.to_string());
            }

            if Authorizer::is_device_sku_authorized(authorizer_info) {
                keys.push(DEVICE_SKU_AUTHORIZED.to_string());
            }

            if Authorizer::is_device_model_authorized(authorizer_info)
                && Authorizer::is_device_make_authorized(authorizer_info)
            {
                keys.push(DEVICE_MAKE_MODEL_AUTHORIZED.to_string());
            }

            let keys_as_str: Vec<&str> = keys.iter().map(String::as_str).collect();
            let device_caps = get_device_capabilities(&state, &keys_as_str).await;
            caps.video_dimensions = device_caps.video_resolution;
            caps.native_dimensions = device_caps.screen_resolution;

            let rp = state.get_receiver_platform();
            let rv = state.get_receiver_version();

            if rp.is_none() || rv.is_none() {
                if let Some(firmware) = device_caps.firmware_info {
                    caps.receiver_platform = Some(firmware.readable.clone());
                    caps.receiver_version = Some(format!(
                        "{}.{}.{}",
                        firmware.major, firmware.minor, firmware.patch
                    ));
                    state.update_receiver_platform(Some(firmware.readable.clone()));
                    state.update_receiver_version(Some(format!(
                        "{}.{}.{}",
                        firmware.major, firmware.minor, firmware.patch
                    )));
                }
            } else {
                caps.receiver_platform = rp;
                caps.receiver_version = rv;
            }

            caps.hdr = device_caps.hdr.map_or_else(
                || {
                    Some(BadgerHDR {
                        settop_hdr_support: vec![],
                        tv_hdr_support: vec![],
                    })
                },
                |hdr_map: HashMap<HdrProfile, bool>| {
                    let hdr_active: Vec<BadgerHdrProfile> = hdr_map
                        .iter()
                        .filter(|&(_, &active)| active)
                        .map(|(&profile, _)| BadgerHdrProfile::from(profile))
                        .collect();

                    Some(BadgerHDR {
                        settop_hdr_support: hdr_active.clone(),
                        tv_hdr_support: hdr_active,
                    })
                },
            );

            caps.hdcp = device_caps.hdcp.map(|hdcp| BadgerHDCP {
                supported_hdcp_version: hdcp.supported_hdcp_version,
                receiver_hdcp_version: hdcp.receiver_hdcp_version,
                current_hdcp_version: hdcp.current_hdcp_version,
                connected: hdcp.is_connected,
                hdcp_compliant: hdcp.is_hdcp_compliant,
                hdcp_enabled: hdcp.is_hdcp_enabled,
                hdcp_reason: hdcp.hdcp_reason,
            });

            caps.web_browser = state.get_web_browser();
            caps.supports_true_sd = Some(true);

            if Authorizer::is_network_status_authorized(authorizer_info) {
                let v = get_cached_is_wifi_device(&state).await;
                caps.is_wifi_device = v;
            }

            let model = state.get_model();
            let make_model = state.get_make_model();
            if model.is_none() || make_model.is_none() {
                caps.model = device_caps.model.clone();
                caps.device_make_model = device_caps
                    .make
                    .as_ref()
                    .zip(device_caps.model.as_ref())
                    .map(|(make, model)| format!("{}_{}", make, model));

                if let Some(make_model) = &caps.device_make_model {
                    state.update_make_model(Some(make_model.clone()));
                }
            } else {
                caps.model = model;
                caps.device_make_model = make_model;
            }

            caps.audio_modes = device_caps.audio.as_ref().and_then(|audio| {
                let supported_audio_modes: Vec<String> = audio
                    .iter()
                    .filter_map(|(k, v)| if *v { Some(k.to_string()) } else { None })
                    .collect();

                if supported_audio_modes.is_empty() {
                    None
                } else {
                    Some(BadgerAudioModes {
                        current_audio_mode: vec![],
                        supported_audio_modes,
                    })
                }
            });
            caps.device_type = Some(state.device_manifest.read().unwrap().get_form_factor());

            return Ok(caps);
        }
        Err(jsonrpsee::core::Error::Custom(String::from(
            "badger.devicecapabilities not available",
        )))
    }
}

#[async_trait]
impl BadgerServiceServer for BadgerServiceImpl {
    async fn badger_info(&self, ctx: CallContext) -> RpcResult<BadgerDeviceInfo> {
        let client = self.state.get_client().clone();

        // check cache first and get the legacy values
        let dev_uid_result = self.legacy_device_uid(ctx.clone()).await;
        let ac_uid_result = self.legacy_account_uid(ctx.clone()).await;
        let postal_code_result = self.legacy_localization_postal_code(ctx.clone()).await;

        let mut info = BadgerDeviceInfo::default();

        if let Some(tz) = client.get_timezone() {
            info.time_zone = Some(tz.time_zone);
            info.time_zone_offset = Some(format_time_zone_offset(tz.offset));
        }

        let permitted = CapsRequest::Permitted(ctx.app_id.clone(), Authorizer::get_info_caps());
        let params = serde_json::to_value(permitted).unwrap();

        let authorizer_info: RpcResult<HashMap<String, bool>> = self
            .state
            .get_service_client()
            .call_and_parse_ripple_main_rpc(
                "ripple.checkCapsRequest",
                Some(params),
                Some(&ctx),
                5000,
                BADGER_SERVICE_ID,
                "Failed to get ripple.checkCapsRequest",
            )
            .await;

        if let Ok(authorizer_info) = authorizer_info {
            debug!(
                "authorizer_info for {} {:?}",
                ctx.clone().app_id,
                authorizer_info
            );
            if Authorizer::is_postal_authorized(&authorizer_info) {
                if let Ok(zip) = postal_code_result {
                    info.zip_code = Some(zip);
                }
            }

            if Authorizer::check_device_info_required(&authorizer_info) {
                if let Ok(caps) = Self::get_device_caps(self, &authorizer_info).await {
                    info.device_capabilities = Some(caps);
                }
            }

            if Authorizer::is_session_required(&authorizer_info) {
                if let Some(session) = get_account_session(&self.state.clone()).await {
                    if Authorizer::is_device_id_authorized(&authorizer_info) {
                        info.device_id = Some(session.device_id);
                    }
                    if Authorizer::is_account_id_authorized(&authorizer_info) {
                        info.account_id = Some(session.account_id);
                    }
                    if Authorizer::is_device_dist_authorized(&authorizer_info) {
                        info.partner_id = Some(session.id);
                    }
                }
            }

            if Authorizer::is_profile_flags_authorized(&authorizer_info) {
                info.user_experience = Some(self.state.get_distributor_experience_id().clone());
            }

            if Authorizer::is_device_uid_authorized(&authorizer_info) {
                if let Ok(uid) = dev_uid_result {
                    info.receiver_id = Some(uid.clone());
                    info.device_hash = Some(uid.clone());
                };
            }

            if Authorizer::is_account_id_authorized(&authorizer_info) {
                if let Ok(uid) = ac_uid_result {
                    info.household_id = Some(uid);
                }
            }

            let result: RpcResult<AppInitParameters> = self
                .state
                .get_service_client()
                .call_and_parse_ripple_main_rpc(
                    "parameters.initialization",
                    None,
                    Some(&ctx),
                    5000,
                    BADGER_SERVICE_ID,
                    "Failed to get parameters.initialization",
                )
                .await;

            match result {
                Ok(params) => {
                    if let Some(us_privacy) = params.us_privacy {
                        info.privacy_settings
                            .insert("us_privacy".into(), us_privacy);
                    }
                    if let Some(lmt) = params.lmt {
                        info.privacy_settings.insert("lmt".into(), lmt.to_string());
                    }
                }
                Err(e) => {
                    error!("Failed to get parameters.initialization response: {:?}", e);
                }
            }
        } else if let Err(e) = authorizer_info {
            error!("Failed to get ripple.checkCapsRequest response: {:?}", e);
        }

        Ok(info)
    }

    async fn badger_shutdown(&self, ctx: CallContext) -> RpcResult<BadgerEmptyResult> {
        let params = serde_json::to_value(CloseRequest {
            reason: CloseReason::UserExit,
        })
        .unwrap();

        let _result: serde_json::Value = self
            .state
            .get_service_client()
            .call_and_parse_ripple_main_rpc(
                "lifecycle.close",
                Some(params),
                Some(&ctx),
                5000,
                BADGER_SERVICE_ID,
                "Error: Could not close app",
            )
            .await?;

        Ok(BadgerEmptyResult::default())
    }

    async fn badger_get_device_id(&self, ctx: CallContext) -> RpcResult<BadgerDialDeviceId> {
        if let Some(session) = get_account_session(&self.state.clone()).await {
            Ok(BadgerDialDeviceId {
                device_id: session.device_id.clone(),
            })
        } else {
            let device_id: String = self
                .state
                .get_service_client()
                .call_and_parse_ripple_main_rpc(
                    "device.id",
                    None,
                    Some(&ctx),
                    5000,
                    BADGER_SERVICE_ID,
                    "Error: Device Id not available",
                )
                .await?;

            Ok(BadgerDialDeviceId { device_id })
        }
    }

    async fn badger_get_device_name(&self, ctx: CallContext) -> RpcResult<BadgerDialDeviceName> {
        let device_name: String = self
            .state
            .get_service_client()
            .call_and_parse_ripple_main_rpc(
                "device.name",
                None,
                Some(&ctx),
                5000,
                BADGER_SERVICE_ID,
                "Error: Device Name not available",
            )
            .await?;

        Ok(BadgerDialDeviceName { device_name })
    }

    async fn badger_network_connectivity(
        &self,
        ctx: CallContext,
    ) -> RpcResult<BadgerNetworkConnectivity> {
        let response: NetworkResponse = self
            .state
            .get_service_client()
            .call_and_parse_ripple_main_rpc(
                "device.network",
                None,
                Some(&ctx),
                5000,
                BADGER_SERVICE_ID,
                "Failed to get network status",
            )
            .await?;

        Ok(BadgerNetworkConnectivity {
            network_interface: Some(
                match response._type {
                    NetworkType::Wifi => "WIFI",
                    NetworkType::Ethernet => "ETHERNET",
                    NetworkType::Hybrid => "ETHERNET",
                }
                .to_string(),
            ),
            status: BadgerNetworkConnectivityStatus::Success,
        })
    }

    async fn legacy_device_uid(&self, ctx: CallContext) -> RpcResult<String> {
        match self
            .state
            .get_device_uid()
            .and_then(|uids| uids.get(&ctx.app_id).cloned())
        {
            Some(uid) => Ok(uid),
            None => {
                let uid_result = get_legacy_uid(
                    &self.state.clone(),
                    UidType::Device,
                    self.state.get_client().clone(),
                    ctx.clone(),
                )
                .await;
                match uid_result {
                    Ok(uid) => {
                        self.state.update_device_uid(Some(HashMap::from([(
                            ctx.app_id.clone(),
                            uid.clone(),
                        )])));
                        Ok(uid)
                    }
                    Err(e) => Err(e),
                }
            }
        }
    }

    async fn legacy_account_uid(&self, ctx: CallContext) -> RpcResult<String> {
        match self
            .state
            .get_account_uid()
            .and_then(|uids| uids.get(&ctx.app_id).cloned())
        {
            Some(uid) => Ok(uid),
            None => {
                match get_legacy_uid(
                    &self.state.clone(),
                    UidType::Account,
                    self.state.get_client().clone(),
                    ctx.clone(),
                )
                .await
                {
                    Ok(uid) => {
                        self.state.update_account_uid(Some(HashMap::from([(
                            ctx.app_id.clone(),
                            uid.clone(),
                        )])));
                        Ok(uid)
                    }
                    Err(e) => Err(e),
                }
            }
        }
    }

    async fn legacy_localization_postal_code(&self, ctx: CallContext) -> RpcResult<String> {
        match self.state.get_postal_code() {
            Some(value) => Ok(value),
            None => {
                let default_values = self.state.get_default();
                let mut country_postal_code_map = default_values.country_postal_code.clone();
                if country_postal_code_map.is_empty() {
                    country_postal_code_map.insert("USA".to_string(), "66952".to_string());
                    country_postal_code_map.insert("US".to_string(), "66952".to_string());
                    country_postal_code_map.insert("CAN".to_string(), "L6T 0C1".to_string());
                    country_postal_code_map.insert("CA".to_string(), "L6T 0C1".to_string());
                }
                let mut country_code = "USA".to_string();

                let result: RpcResult<serde_json::Value> = self
                    .state
                    .get_service_client()
                    .call_and_parse_ripple_main_rpc(
                        "localization.countryCode",
                        None,
                        Some(&ctx),
                        5000,
                        BADGER_SERVICE_ID,
                        "Failed to get localization.countryCode",
                    )
                    .await;

                if let Ok(val) = result {
                    if let Some(code) = val.as_str() {
                        debug!("localization.countryCode result: {:?}", code);
                        country_code = code.to_owned();
                    } else {
                        error!("Failed to parse country code from response: {:?}", val);
                    }
                } else if let Err(e) = result {
                    error!("Failed to get localization.countryCode response: {:?}", e);
                }

                if let Some(pc) = country_postal_code_map.get(&country_code) {
                    self.state.update_postal_code(Some(pc.to_string()));
                    Ok(pc.to_string())
                } else {
                    self.state.update_postal_code(None);
                    Err(jsonrpsee::core::Error::Custom(
                        "Error: failed to get postal code".into(),
                    ))
                }
            }
        }
    }

    async fn device_capabilities(&self, ctx: CallContext) -> RpcResult<BadgerDeviceCapabilities> {
        let permitted = CapsRequest::Permitted(ctx.app_id.clone(), Authorizer::get_device_caps());
        let params = serde_json::to_value(permitted).unwrap();

        let authorizer_info: HashMap<String, bool> = self
            .state
            .get_service_client()
            .call_and_parse_ripple_main_rpc(
                "ripple.checkCapsRequest",
                Some(params),
                Some(&ctx),
                5000,
                BADGER_SERVICE_ID,
                "badger.devicecapabilities not available",
            )
            .await?;

        Self::get_device_caps(self, &authorizer_info).await
    }

    async fn show_toaster(&self, _ctx: CallContext) -> RpcResult<DefaultResponse> {
        Ok(DefaultResponse {})
    }

    async fn badger_dismiss_loading_screen(
        &self,
        ctx: CallContext,
    ) -> RpcResult<BadgerEmptyResult> {
        let _result: serde_json::Value = self
            .state
            .get_service_client()
            .call_and_parse_ripple_main_rpc(
                "lifecycle.ready",
                None,
                Some(&ctx),
                5000,
                BADGER_SERVICE_ID,
                "Error: Could not put app in ready state",
            )
            .await?;

        Ok(BadgerEmptyResult::default())
    }
    async fn badger_get_payload(&self, ctx: CallContext) -> RpcResult<BadgerDialPayload> {
        let result: Result<String, _> = self
            .state
            .get_service_client()
            .call_and_parse_ripple_main_rpc(
                "ripple.getSecondScreenPayload",
                None,
                Some(&ctx),
                5000,
                BADGER_SERVICE_ID,
                "Error: Could not get second screen payload",
            )
            .await;

        if result.is_ok() {
            let payload = result.unwrap();
            return Ok(BadgerDialPayload {
                payload: if !payload.is_empty() {
                    Some(payload.to_owned())
                } else {
                    None
                },
            });
        }

        Ok(BadgerDialPayload::default())
    }

    async fn badger_on_launch(
        &self,
        ctx: CallContext,
        request: BadgerLaunchCallback,
    ) -> RpcResult<ListenerResponse> {
        let listen = request.on_launch_callback;
        let event = AppEventRequest::Register(
            ctx,
            SECOND_SCREEN_EVENT_ON_LAUNCH_REQUEST.into(),
            request.into(),
        );
        let _ = self
            .state
            .get_service_client()
            .request_with_timeout_main(
                "ripple.sendAppEventRequest".to_string(),
                Some(serde_json::to_value(event).unwrap()),
                None,
                5000,
                BADGER_SERVICE_ID.to_string(),
                None,
            )
            .await;
        Ok(ListenerResponse {
            listening: listen,
            event: SECOND_SCREEN_EVENT_ON_LAUNCH_REQUEST.into(),
        })
    }

    async fn badger_navigate_to_company_page(
        &self,
        _ctx: CallContext,
        req: NavigateCompanyPageRequest,
    ) -> RpcResult<BadgerEmptyResult> {
        let app_id = self
            .state
            .device_manifest
            .read()
            .unwrap()
            .applications
            .defaults
            .main
            .clone();
        let intent = NavigationIntent::NavigationIntentStrict(NavigationIntentStrict::Section(
            SectionIntent {
                context: DiscoveryContext {
                    source: String::from("system"),
                    age_policy: None,
                },
                data: SectionIntentData {
                    section_name: format!("company:{}", req.company_id),
                },
            },
        ));
        let event = AppEventRequest::Emit(AppEvent {
            app_id: Some(app_id),
            event_name: "discovery.onNavigateTo".into(),
            context: None,
            result: serde_json::to_value(intent).unwrap(),
        });

        let _ = self
            .state
            .get_service_client()
            .request_with_timeout_main(
                "ripple.sendAppEventRequest".to_string(),
                Some(serde_json::to_value(event).unwrap()),
                None,
                5000,
                BADGER_SERVICE_ID.to_string(),
                None,
            )
            .await;
        Ok(BadgerEmptyResult::default())
    }

    async fn prompt_email(
        &self,
        ctx: CallContext,
        request: PromptEmailRequest,
    ) -> RpcResult<PromptEmailResult> {
        let keyboard_req = KeyboardSessionRequest {
            _type: KeyboardType::Email,
            message: match request.prefill_type {
                PrefillType::SignIn => "Sign in".to_string(),
                PrefillType::SignUp => "Sign up".to_string(),
            },
            ctx: ctx.clone(),
        };
        let mut client = self.state.get_service_client().clone();
        // TBD: Define Ripple main rpc call to get the keyboard session
        let resp: RpcResult<KeyboardSessionResponse> = client
            .call_and_parse_ripple_main_rpc(
                "ripple.promptEmailRequest",
                Some(serde_json::to_value(keyboard_req).unwrap()),
                Some(&ctx),
                5000,
                BADGER_SERVICE_ID,
                "Error: Keyboard not available".into(),
            )
            .await;

        match resp {
            Ok(resp) => {
                let status = match resp.canceled {
                    true => PromptEmailStatus::Dismiss,
                    false => PromptEmailStatus::Success,
                };
                return Ok(PromptEmailResult {
                    status,
                    data: PromptEmailData { email: resp.text },
                });
            }
            Err(e) => {
                error!("Failed to get keyboard session response: {:?}", e);
                Err(jsonrpsee::core::Error::Custom(
                    "Error: Keyboard not available".into(),
                ))
            }
        }
    }

    async fn badger_show_pin_overlay(
        &self,
        ctx: CallContext,
        request: ShowPinOverlayRequest,
    ) -> RpcResult<BadgerPinOverlayResponse> {
        // TBD: Define Ripple main rpc call for pin challenge

        let pin_space = from_badger_pin_type(&request.pin_type);
        let app_id = ctx.app_id.clone();
        let pin_challenge_req = PinChallengeRequestWithContext {
            pin_space,
            requestor: ChallengeRequestor {
                id: app_id.clone(),
                name: app_id.clone(),
            },
            capability: None,
            call_ctx: ctx.clone(),
        };
        let mut client = self.state.get_service_client().clone();
        let pin_resp: RpcResult<PinChallengeResponse> = client
            .call_and_parse_ripple_main_rpc(
                "ripple.showPinOverlay",
                Some(serde_json::to_value(pin_challenge_req).unwrap()),
                Some(&ctx),
                180000,
                BADGER_SERVICE_ID,
                "Error: Failed to get pin overlay".into(),
            )
            .await;
        if let Ok(resp) = pin_resp {
            debug!("Pin challenge response: {:?}", resp);
            let status = BadgerPinStatus::from_result_reason(resp.reason);
            let msg: String = status.to_string(&request.pin_type);
            if !status.is_error() {
                return Ok(BadgerPinOverlayResponse {
                    status,
                    message: msg,
                });
            }
        }
        Err(jsonrpsee::core::Error::Custom(
            "Error: Pin not available".into(),
        ))
    }

    async fn settings(
        &self,
        ctx: CallContext,
        req: BadgerSettingsRequest,
    ) -> RpcResult<HashMap<String, SettingValue>> {
        if req.keys.is_empty() {
            return Err(jsonrpsee::core::Error::Custom(
                "Error: Couldnt retrieve settings".into(),
            ));
        }
        let mut keys = Vec::new();
        let mut key_map = HashMap::new();
        for key in req.keys {
            if let Some(sk) = from_key(&key) {
                keys.push(sk.clone());
                key_map.insert(sk.to_string(), key.clone());
            } else {
                return Err(jsonrpsee::core::Error::Custom(
                    "Error: Couldnt retrieve settings".into(),
                ));
            }
        }
        debug!("{:?}", key_map);
        if !keys.is_empty() {
            // TBD: Define Ripple main rpc call to get the settings
            let request = SettingsRequest::Get(SettingsRequestParam::new(ctx.clone(), keys, None));
            let resp: RpcResult<HashMap<String, SettingValue>> = self
                .state
                .get_service_client()
                .call_and_parse_ripple_main_rpc(
                    "ripple.getSettingsRequest",
                    Some(serde_json::to_value(request).unwrap()),
                    Some(&ctx),
                    5000,
                    BADGER_SERVICE_ID,
                    "Error: Couldnt retrieve settings".into(),
                )
                .await;

            debug!("Settings response: {:?}", resp);

            if let Ok(r) = resp {
                return Ok(r
                    .iter()
                    .filter(|(k, _)| key_map.contains_key(*k))
                    .map(|(k, v)| (key_map.get(k).unwrap().clone(), v.clone()))
                    .collect());
            }
        }

        Err(jsonrpsee::core::Error::Custom(
            "Error: Couldnt retrieve settings".into(),
        ))
    }

    async fn subscribe_to_settings(
        &self,
        ctx: CallContext,
        req: BadgerSettingsRequest,
    ) -> RpcResult<()> {
        if req.keys.is_empty() {
            return Err(jsonrpsee::core::Error::Custom(
                "Error: Couldnt retrieve settings".into(),
            ));
        }

        let mut keys = Vec::new();
        let mut alias_map = HashMap::new();
        for key in req.keys {
            if let Some(sk) = from_key(&key) {
                alias_map.insert(sk.to_string(), key.clone());
                keys.push(sk);
            } else {
                return Err(jsonrpsee::core::Error::Custom(
                    "Error: Couldnt retrieve settings".into(),
                ));
            }
        }
        if keys.is_empty() {
            return Err(jsonrpsee::core::Error::Custom(
                "Error: Couldnt retrieve settings".into(),
            ));
        }

        let _: () = self
            .state
            .get_service_client()
            .call_and_parse_ripple_main_rpc(
                "ripple.subscribeSettings",
                Some(
                    serde_json::to_value(SettingsRequestParam::new(
                        ctx.clone(),
                        keys,
                        Some(alias_map),
                    ))
                    .unwrap(),
                ),
                None,
                5000,
                BADGER_SERVICE_ID,
                "Error: Couldnt retrieve settings",
            )
            .await?;
        Ok(())
    }
}

async fn get_legacy_uid(
    state: &BadgerState,
    uid_type: UidType,
    client: ExtnClient,
    ctx: CallContext,
) -> RpcResult<String> {
    let magic = client.get_config("magic");

    if magic.is_none() {
        return Err(CustomErr("Error: missing magic".into()));
    }

    match get_account_session(state).await {
        Some(session) => {
            let reference: String = match uid_type {
                UidType::Account => session.account_id,
                UidType::Device => session.device_id,
            };

            if let Some(id) = salt_using_app_scope(ctx.app_id, reference, magic) {
                Ok(id)
            } else {
                Err(CustomErr("Error: failed to get account session".into()))
            }
        }
        None => Err(CustomErr("Error: failed to get account session".into())),
    }
}

pub(crate) async fn get_account_session(state: &BadgerState) -> Option<AccountSession> {
    if let Some(session) = state.get_account_session() {
        return Some(session);
    }

    match ThunderSessionService::get_account_session(state.get_state()).await {
        Ok(session) => {
            state.update_account_session(Some(session.clone()));
            Some(session)
        }
        Err(_) => {
            error!("Error: failed to get account session");
            state.update_account_session(None);
            None
        }
    }
}

fn format_time_zone_offset(offset: i64) -> String {
    let (sign, abs_offset) = if offset < 0 {
        ('-', -offset)
    } else {
        ('+', offset)
    };

    let hours = abs_offset / 3600;
    let minutes = (abs_offset % 3600) / 60;

    format!("{}{}:{:02}", sign, hours, minutes)
}

/*
#[cfg(test)]
mod tests {
    use super::*;
    use crate::handlers::badger_info_rpc::serde_json::Value;
    use crate::test_utils::test_utils::TestFixture;
    use jsonrpsee::core::Error;
    use ripple_sdk::Mockable;
    use ripple_sdk::{
        api::{
            firebolt::{fb_keyboard::KeyboardSessionResponse, fb_pin::PinChallengeResponse},
            manifest::device_manifest::{AppLibraryEntry, AppManifestLoad, BootState},
        },
        tokio,
        utils::error::RippleError,
    };
    use rstest::{fixture, rstest};
    use serde_json::json;
    use std::collections::HashMap;
    const HDCP_REASON_RX_DEV_ON: u32 = 1; //Rx device is connected with power ON state, and HDCP authentication is not initiated

    #[derive(Debug)]
    struct ValidationError(String);

    impl ValidationError {
        #[allow(dead_code)]
        pub fn new(message: &str) -> Self {
            ValidationError(message.to_string())
        }
    }

    impl std::fmt::Display for ValidationError {
        fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
            write!(f, "{}", self.0)
        }
    }

    impl BadgerDeviceCapabilities {
        fn validate(&self, expected: &BadgerDeviceCapabilities) -> Result<(), ValidationError> {
            // Validate device capabilities
            self.validate_device_capabilities(expected)?;
            Ok(())
        }

        fn validate_device_capabilities(
            &self,
            expected_capabilities: &BadgerDeviceCapabilities,
        ) -> Result<(), ValidationError> {
            let actual_capabilities = &self;
            // Validate is_wifi_device
            if let Some(expected_is_wifi_device) = expected_capabilities.is_wifi_device {
                if let Some(actual_is_wifi_device) = actual_capabilities.is_wifi_device {
                    if actual_is_wifi_device != expected_is_wifi_device {
                        return Err(ValidationError(format!(
                            "Is WiFi device mismatch. Expected: {}, Actual: {}",
                            expected_is_wifi_device, actual_is_wifi_device
                        )));
                    }
                }
            }

            // Validate native dimensions
            if let Some(expected_native_dimensions) = &expected_capabilities.native_dimensions {
                if let Some(actual_native_dimensions) = &actual_capabilities.native_dimensions {
                    if actual_native_dimensions != expected_native_dimensions {
                        return Err(ValidationError(format!(
                            "Native dimensions mismatch. Expected: {:?}, Actual: {:?}",
                            expected_native_dimensions, actual_native_dimensions
                        )));
                    }
                }
            }

            // Validate model
            if let Some(expected_model) = &expected_capabilities.model {
                if let Some(actual_model) = &actual_capabilities.model {
                    if actual_model != expected_model {
                        return Err(ValidationError(format!(
                            "Model mismatch. Expected: {}, Actual: {}",
                            expected_model, actual_model
                        )));
                    }
                }
            }

            // Validate receiver platform
            if let Some(expected_receiver_platform) = &expected_capabilities.receiver_platform {
                if let Some(actual_receiver_platform) = &actual_capabilities.receiver_platform {
                    if actual_receiver_platform != expected_receiver_platform {
                        return Err(ValidationError(format!(
                            "Receiver platform mismatch. Expected: {}, Actual: {}",
                            expected_receiver_platform, actual_receiver_platform
                        )));
                    }
                }
            }

            // Validate receiver version
            if let Some(expected_receiver_version) = &expected_capabilities.receiver_version {
                if let Some(actual_receiver_version) = &actual_capabilities.receiver_version {
                    if actual_receiver_version != expected_receiver_version {
                        return Err(ValidationError(format!(
                            "Receiver version mismatch. Expected: {}, Actual: {}",
                            expected_receiver_version, actual_receiver_version
                        )));
                    }
                }
            }

            // Validate supports_true_sd
            if let Some(expected_supports_true_sd) = expected_capabilities.supports_true_sd {
                if let Some(actual_supports_true_sd) = actual_capabilities.supports_true_sd {
                    if actual_supports_true_sd != expected_supports_true_sd {
                        return Err(ValidationError(format!(
                            "Supports True SD mismatch. Expected: {}, Actual: {}",
                            expected_supports_true_sd, actual_supports_true_sd
                        )));
                    }
                }
            }

            // Validate device_make_model
            if let Some(expected_device_make_model) = &expected_capabilities.device_make_model {
                if let Some(actual_device_make_model) = &actual_capabilities.device_make_model {
                    if actual_device_make_model != expected_device_make_model {
                        return Err(ValidationError(format!(
                            "Device make model mismatch. Expected: {}, Actual: {}",
                            expected_device_make_model, actual_device_make_model
                        )));
                    }
                }
            }

            // Validate device type
            if let Some(expected_device_type) = &expected_capabilities.device_type {
                if let Some(actual_device_type) = &actual_capabilities.device_type {
                    if actual_device_type != expected_device_type {
                        return Err(ValidationError(format!(
                            "Device type mismatch. Expected: {}, Actual: {}",
                            expected_device_type, actual_device_type
                        )));
                    }
                }
            }

            // Validate video dimensions
            if let Some(expected_dimensions) = &expected_capabilities.video_dimensions {
                if let Some(actual_dimensions) = &actual_capabilities.video_dimensions {
                    if actual_dimensions != expected_dimensions {
                        return Err(ValidationError(format!(
                            "Video dimensions mismatch. Expected: {:?}, Actual: {:?}",
                            expected_dimensions, actual_dimensions
                        )));
                    }
                }
            }

            // Validate HDR support
            if let Some(expected_hdr) = &expected_capabilities.hdr {
                if let Some(actual_hdr) = &actual_capabilities.hdr {
                    for expected_profile in &expected_hdr.settop_hdr_support {
                        if !actual_hdr.settop_hdr_support.contains(expected_profile) {
                            return Err(ValidationError(format!(
                                    "Settop HDR support mismatch. Expected profile: {:?} not found in actual profiles: {:?}",
                                    expected_profile, actual_hdr.settop_hdr_support
                                )));
                        }
                    }
                    for expected_profile in &expected_hdr.tv_hdr_support {
                        if !actual_hdr.tv_hdr_support.contains(expected_profile) {
                            return Err(ValidationError(format!(
                                    "TV HDR support mismatch. Expected profile: {:?} not found in actual profiles: {:?}",
                                    expected_profile, actual_hdr.tv_hdr_support
                                )));
                        }
                    }
                }
            }

            // Validate HDCP support
            if let Some(expected_hdcp) = &expected_capabilities.hdcp {
                if let Some(actual_hdcp) = &actual_capabilities.hdcp {
                    if actual_hdcp.supported_hdcp_version != expected_hdcp.supported_hdcp_version {
                        return Err(ValidationError(format!(
                            "Supported HDCP version mismatch. Expected: {}, Actual: {}",
                            expected_hdcp.supported_hdcp_version,
                            actual_hdcp.supported_hdcp_version
                        )));
                    }
                    if actual_hdcp.receiver_hdcp_version != expected_hdcp.receiver_hdcp_version {
                        return Err(ValidationError(format!(
                            "Receiver HDCP version mismatch. Expected: {}, Actual: {}",
                            expected_hdcp.receiver_hdcp_version, actual_hdcp.receiver_hdcp_version
                        )));
                    }
                    if actual_hdcp.current_hdcp_version != expected_hdcp.current_hdcp_version {
                        return Err(ValidationError(format!(
                            "Current HDCP version mismatch. Expected: {}, Actual: {}",
                            expected_hdcp.current_hdcp_version, actual_hdcp.current_hdcp_version
                        )));
                    }
                    if actual_hdcp.connected != expected_hdcp.connected {
                        return Err(ValidationError(format!(
                            "HDCP connected status mismatch. Expected: {}, Actual: {}",
                            expected_hdcp.connected, actual_hdcp.connected
                        )));
                    }
                    if actual_hdcp.hdcp_compliant != expected_hdcp.hdcp_compliant {
                        return Err(ValidationError(format!(
                            "HDCP compliant status mismatch. Expected: {}, Actual: {}",
                            expected_hdcp.hdcp_compliant, actual_hdcp.hdcp_compliant
                        )));
                    }
                    if actual_hdcp.hdcp_enabled != expected_hdcp.hdcp_enabled {
                        return Err(ValidationError(format!(
                            "HDCP enabled status mismatch. Expected: {}, Actual: {}",
                            expected_hdcp.hdcp_enabled, actual_hdcp.hdcp_enabled
                        )));
                    }
                }
            }

            // Validate web browser
            if let Some(expected_web_browser) = &expected_capabilities.web_browser {
                if let Some(actual_web_browser) = &actual_capabilities.web_browser {
                    if actual_web_browser.user_agent != expected_web_browser.user_agent {
                        return Err(ValidationError(format!(
                            "Web browser user agent mismatch. Expected: {}, Actual: {}",
                            expected_web_browser.user_agent, actual_web_browser.user_agent
                        )));
                    }
                    if actual_web_browser.version != expected_web_browser.version {
                        return Err(ValidationError(format!(
                            "Web browser version mismatch. Expected: {}, Actual: {}",
                            expected_web_browser.version, actual_web_browser.version
                        )));
                    }
                    if actual_web_browser.browser_type != expected_web_browser.browser_type {
                        return Err(ValidationError(format!(
                            "Web browser type mismatch. Expected: {}, Actual: {}",
                            expected_web_browser.browser_type, actual_web_browser.browser_type
                        )));
                    }
                }
            }

            // Validate audio modes
            if let Some(expected_audio_modes) = &expected_capabilities.audio_modes {
                if let Some(actual_audio_modes) = &actual_capabilities.audio_modes {
                    for actual_mode in &actual_audio_modes.current_audio_mode {
                        if !expected_audio_modes
                            .current_audio_mode
                            .contains(actual_mode)
                        {
                            return Err(ValidationError(format!(
                                "Unexpected current audio mode: {}",
                                actual_mode
                            )));
                        }
                    }

                    for actual_mode in &actual_audio_modes.supported_audio_modes {
                        if !expected_audio_modes
                            .supported_audio_modes
                            .contains(actual_mode)
                        {
                            return Err(ValidationError(format!(
                                "Unexpected supported audio mode: {}",
                                actual_mode
                            )));
                        }
                    }
                }
            }

            Ok(())
        }
    }

    impl BadgerDeviceInfo {
        fn validate(&self, expected: &BadgerDeviceInfo) -> Result<(), ValidationError> {
            // Validate top-level fields
            self.validate_identifiers(expected)?;
            self.validate_time_zone(expected)?;
            self.validate_user_experience(expected)?;

            // Validate device capabilities if present
            if let (Some(actual_capabilities), Some(expected_capabilities)) =
                (&self.device_capabilities, &expected.device_capabilities)
            {
                actual_capabilities.validate_device_capabilities(expected_capabilities)?;
            }

            Ok(())
        }

        fn validate_identifiers(&self, expected: &BadgerDeviceInfo) -> Result<(), ValidationError> {
            // Validate specific identifiers
            if let Some(expected_receiver_id) = &expected.receiver_id {
                if let Some(actual_receiver_id) = &self.receiver_id {
                    if actual_receiver_id != expected_receiver_id {
                        return Err(ValidationError(format!(
                            "Receiver ID mismatch. Expected: {}, Actual: {}",
                            expected_receiver_id, actual_receiver_id
                        )));
                    }
                }
            }

            if let Some(expected_device_id) = &expected.device_id {
                if let Some(actual_device_id) = &self.device_id {
                    if actual_device_id != expected_device_id {
                        return Err(ValidationError(format!(
                            "Device ID mismatch. Expected: {}, Actual: {}",
                            expected_device_id, actual_device_id
                        )));
                    }
                }
            }

            if let Some(expected_account_id) = &expected.account_id {
                if let Some(actual_account_id) = &self.account_id {
                    if actual_account_id != expected_account_id {
                        return Err(ValidationError(format!(
                            "Account ID mismatch. Expected: {}, Actual: {}",
                            expected_account_id, actual_account_id
                        )));
                    }
                }
            }

            // Add similar validations for other identifiers like household_id, device_hash, etc.

            Ok(())
        }

        fn validate_time_zone(&self, expected: &BadgerDeviceInfo) -> Result<(), ValidationError> {
            // Validate zip code
            if let Some(expected_zip_code) = &expected.zip_code {
                if let Some(actual_zip_code) = &self.zip_code {
                    if actual_zip_code != expected_zip_code {
                        return Err(ValidationError(format!(
                            "Zip code mismatch. Expected: {}, Actual: {}",
                            expected_zip_code, actual_zip_code
                        )));
                    }
                }
            }

            // Validate time zone
            if let Some(expected_time_zone) = &expected.time_zone {
                if let Some(actual_time_zone) = &self.time_zone {
                    if actual_time_zone != expected_time_zone {
                        return Err(ValidationError(format!(
                            "Time zone mismatch. Expected: {}, Actual: {}",
                            expected_time_zone, actual_time_zone
                        )));
                    }
                }
            }

            // Validate time zone offset
            if let Some(expected_time_zone_offset) = &expected.time_zone_offset {
                if let Some(actual_time_zone_offset) = &self.time_zone_offset {
                    if actual_time_zone_offset != expected_time_zone_offset {
                        return Err(ValidationError(format!(
                            "Time zone offset mismatch. Expected: {}, Actual: {}",
                            expected_time_zone_offset, actual_time_zone_offset
                        )));
                    }
                }
            }

            Ok(())
        }

        fn validate_user_experience(
            &self,
            expected: &BadgerDeviceInfo,
        ) -> Result<(), ValidationError> {
            // Validate user experience
            if let Some(expected_user_experience) = &expected.user_experience {
                if let Some(actual_user_experience) = &self.user_experience {
                    if actual_user_experience != expected_user_experience {
                        return Err(ValidationError(format!(
                            "User experience mismatch. Expected: {}, Actual: {}",
                            expected_user_experience, actual_user_experience
                        )));
                    }
                }
            }

            Ok(())
        }
    }

    #[macro_export]
    macro_rules! test_extn_response_with_params {
        ($test_name:ident, $method:ident, $mock_payload:expr,  $params:expr, $assertion:expr) => {
            #[tokio::test(flavor = "multi_thread")]
            async fn $test_name() {
                let fixture = TestFixture::new(true);
                fixture.queue_mock_extn_response($mock_payload);

                let badger_info = fixture.create_badger_info();
                let result = badger_info.$method(fixture.ctx, $params).await;

                // Ensure the result is successful before applying the assertion
                assert!(
                    result.is_ok(),
                    "[{}] Expected Ok, but got Err({:?})",
                    stringify!($test_name),
                    result
                );
                // Pass the successful value to the assertion
                ($assertion)(result.unwrap());
            }
        };
    }

    #[macro_export]
    macro_rules! test_extn_response {
        ($test_name:ident, $method:ident, $mock_payload:expr, $assertion:expr) => {
            #[tokio::test(flavor = "multi_thread")]
            async fn $test_name() {
                let fixture = TestFixture::only_session_mock();

                if let ExtnResponse::Float(0.0) = $mock_payload {
                    // Do nothing, as the mock_payload is ExtnResponse::None
                } else {
                    fixture.queue_mock_extn_response($mock_payload);
                }

                let badger_info = fixture.create_badger_info();
                let result = badger_info.$method(fixture.ctx).await;

                // Ensure the result is successful before applying the assertion
                assert!(
                    result.is_ok(),
                    "[{}] Expected Ok, but got Err({:?})",
                    stringify!($test_name),
                    result
                );
                // Pass the successful value to the assertion
                ($assertion)(result.unwrap());
            }
        };
    }

    #[macro_export]
    macro_rules! test_with_cached_state_new {
        ($test_name:ident, $method:ident, $mock_payload:expr, $expected:expr) => {
            #[tokio::test(flavor = "multi_thread")]
            async fn $test_name() {
                let fixture = TestFixture::with_mocked_state();
                fixture.queue_mock_extn_response($mock_payload);
                fixture.queue_mock_extn_response(ExtnResponse::None(()));

                let badger_info = fixture.create_badger_info();
                let result = badger_info.$method(fixture.ctx).await;

                assert!(result.is_ok(), "Method call failed");
                let unwrapped_result = result.unwrap();
                println!("unwrapped {:?}", unwrapped_result);
                // Validate against expected result
                match unwrapped_result.validate($expected) {
                    Ok(_) => {
                        println!("Validation successful for test {}", stringify!($test_name));
                    }
                    Err(validation_error) => {
                        println!(
                            "Validation failed for test {}: {}",
                            stringify!($test_name),
                            validation_error
                        );
                        panic!("Validation failed");
                    }
                }
            }
        };
    }

    #[macro_export]
    macro_rules! test_with_multiple_mocks {
        ($test_name:ident, $method:ident, $mocks:expr, $assertion:expr) => {
            #[tokio::test(flavor = "multi_thread")]
            async fn $test_name() {
                let fixture = {
                    let fixture = TestFixture::only_session_mock();
                    let fixture = Arc::new(Mutex::new(fixture));
                    fixture
                };

                for mock_payload in $mocks {
                    fixture.lock().await.queue_mock_extn_response(mock_payload);
                }

                let badger_info = fixture.lock().await.create_badger_info();
                let result = badger_info.$method(fixture.lock().await.ctx.clone()).await;

                // Ensure the result is successful before applying the assertion
                assert!(
                    result.is_ok(),
                    "[{}] Expected Ok, but got Err({:?})",
                    stringify!($test_name),
                    result
                );
                // Pass the successful value to the assertion
                ($assertion)(result.unwrap());
            }
        };
    }

    // tests for legacy_account_uid
    test_extn_response!(
        test_legacy_account_uid_success,
        legacy_account_uid,
        ExtnResponse::Float(0.0),
        |uid: String| assert_eq!(uid, "1dd79201fa7e7619a55072f6a98a22731c83539f".to_string())
    );

    #[fixture]
    fn failure_methods_data() -> Vec<String> {
        vec![
            "badger_get_device_name".to_string(),
            "badger_network_connectivity".to_string(),
            "show_toaster".to_string(),
            "badger_shutdown".to_string(),
            "badger_dismiss_loading_screen".to_string(),
            "badger_get_payload".to_string(),
            "device_capabilities".to_string(),
        ]
    }

    async fn test_method_failure(
        badger_info: &BadgerInfoImpl,
        fixture: &TestFixture,
        method: &str,
    ) {
        let result = match method {
            "badger_get_device_name" => badger_info
                .badger_get_device_name(fixture.ctx.clone())
                .await
                .map(|res| res.device_name),
            "badger_network_connectivity" => badger_info
                .badger_network_connectivity(fixture.ctx.clone())
                .await
                .map(|_| "Success".to_string()),
            "show_toaster" => badger_info
                .show_toaster(fixture.ctx.clone())
                .await
                .map(|_| "Success".to_string()),
            "badger_shutdown" => badger_info
                .badger_shutdown(fixture.ctx.clone())
                .await
                .map(|_| "Success".to_string()),
            "badger_dismiss_loading_screen" => badger_info
                .badger_dismiss_loading_screen(fixture.ctx.clone())
                .await
                .map(|_| "Success".to_string()),
            "badger_get_payload" => badger_info
                .badger_get_payload(fixture.ctx.clone())
                .await
                .map(|res| res.payload.unwrap_or_default()),
            "device_capabilities" => badger_info
                .device_capabilities(fixture.ctx.clone())
                .await
                .map(|_| "Success".to_string()),
            _ => panic!("Method not supported: {}", method),
        };

        if method == "show_toaster" || method == "badger_get_payload" {
            assert!(
                result.is_ok(),
                "[{}] Expected Ok, but got Err({:?})",
                method,
                result
            );
        } else {
            assert!(
                result.is_err(),
                "[{}] Expected Err, but got Ok({:?})",
                method,
                result
            );
        }
    }

    #[rstest]
    #[tokio::test(flavor = "multi_thread")]
    async fn test_failures(#[from(failure_methods_data)] method: Vec<String>) {
        let fixture = TestFixture::only_session_mock();
        let badger_info = fixture.create_badger_info();
        let errors = vec![
            RippleError::ParseError,
            RippleError::TimeoutError,
            RippleError::InvalidOutput,
        ];

        for error in errors {
            if error == RippleError::InvalidOutput {
                fixture.queue_mock_extn_response(ExtnResponse::Number(123));
            } else {
                fixture.queue_error_resp(error.clone());
            }

            for m in method.iter() {
                test_method_failure(&badger_info, &fixture, m).await;
            }
        }
    }

    #[fixture]
    fn failure_param_methods_data() -> Vec<String> {
        vec![
            "badger_show_pin_overlay".to_string(),
            "settings".to_string(),
            "subscribe_to_settings".to_string(),
            "prompt_email".to_string(),
        ]
    }

    async fn test_method_param_failure(
        badger_info: &BadgerInfoImpl,
        fixture: &TestFixture,
        method: &str,
    ) {
        let result: Result<bool, Error> = match method {
            "badger_show_pin_overlay" => badger_info
                .badger_show_pin_overlay(
                    fixture.ctx.clone(),
                    ShowPinOverlayRequest {
                        pin_type: "purchase_pin".to_string(),
                        suppress_snooze: None,
                    },
                )
                .await
                .map(|_| true)
                .map_err(|e| e.into()),
            "settings" => badger_info
                .settings(
                    fixture.ctx.clone(),
                    BadgerSettingsRequest {
                        keys: vec!["VOICE_GUIDANCE_STATE".to_string()],
                    },
                )
                .await
                .map(|_| true)
                .map_err(|e| e.into()),
            "subscribe_to_settings" => badger_info
                .subscribe_to_settings(
                    fixture.ctx.clone(),
                    BadgerSettingsRequest {
                        keys: vec!["VOICE_GUIDANCE_STATE".to_string()],
                    },
                )
                .await
                .map(|_| true)
                .map_err(|e| e.into()),
            "prompt_email" => badger_info
                .prompt_email(
                    fixture.ctx.clone(),
                    PromptEmailRequest {
                        prefill_type: PrefillType::SignUp,
                    },
                )
                .await
                .map(|_| true)
                .map_err(|e| e.into()),
            "badger_navigate_to_company_page" => badger_info
                .badger_navigate_to_company_page(
                    fixture.ctx.clone(),
                    NavigateCompanyPageRequest {
                        company_id: "company123".to_string(),
                    },
                )
                .await
                .map(|_| true)
                .map_err(|e| e.into()),
            _ => panic!("Method not supported: {}", method),
        };

        if method == "badger_metrics_handler" {
            assert!(
                result.is_ok(),
                "[{}] Expected Ok, but got Err({:?})",
                method,
                result
            );
        } else {
            assert!(
                result.is_err(),
                "[{}] Expected Err, but got Ok({:?})",
                method,
                result
            );
        }
    }

    #[rstest]
    #[tokio::test(flavor = "multi_thread")]
    async fn test_param_failures(#[from(failure_param_methods_data)] method: Vec<String>) {
        let fixture = TestFixture::new(true);
        let badger_info = fixture.create_badger_info();

        let errors = vec![
            RippleError::ParseError,
            RippleError::TimeoutError,
            RippleError::InvalidOutput,
        ];

        for error in errors {
            if error == RippleError::InvalidOutput {
                fixture.queue_mock_extn_response(ExtnResponse::Number(123));
            } else {
                fixture.queue_error_resp(error.clone());
            }

            for m in method.iter() {
                test_method_param_failure(&badger_info, &fixture, m).await;
            }
        }
    }

    // tests for legacy_device_uid
    test_extn_response!(
        test_legacy_device_uid_success,
        legacy_device_uid,
        ExtnResponse::Float(0.0),
        |uid: String| assert_eq!(uid, "246815e30802f5fc4a2883576943a77256b9df5f".to_string())
    );

    // TODO: fix test after implementing thunder mock
    // #[rstest]
    // #[case::test_legacy_device_uid_parse_failure(
    //     "test_legacy_device_uid_parse_failure".to_string(),
    //     RippleError::ParseError
    // )]
    // #[case::test_legacy_device_uid_timeout_failure(
    //     "test_legacy_device_uid_timeout_failure".to_string(),
    //     RippleError::TimeoutError
    // )]
    // #[case::test_legacy_device_uid_unexpected_response_failure(
    //     "test_legacy_device_uid_unexpected_response_failure".to_string(),
    //     RippleError::TimeoutError
    // )]
    // async fn test_legacy_device_uid_failure(#[case] test_name: String, #[case] error: RippleError) {
    //     let fixture = TestFixture::only_session_mock();
    //     let badger_info = fixture.create_badger_info();

    //     if test_name.contains("unexpected") {
    //         fixture.queue_mock_extn_response(ExtnResponse::Value(Value::String(
    //             "unexpected".to_string(),
    //         )));
    //     } else {
    //         fixture.queue_error_resp(error);
    //     }

    //     let result = badger_info.legacy_device_uid(fixture.ctx).await;
    //     println!("**** result: {:?}", result);

    //     // Ensure the result is an error before applying the assertion
    //     assert!(
    //         result.is_err(),
    //         "[{}] Expected Err, but got Ok({:?})",
    //         test_name,
    //         result
    //     );

    //     assert!(result.is_err());
    // }

    // tests for badger_get_device_name
    test_extn_response!(
        test_badger_get_device_name_success,
        badger_get_device_name,
        ExtnResponse::Value(Value::String("Test Device".to_string(),)),
        |resp: BadgerDialDeviceName| assert_eq!(resp.device_name, "Test Device".to_string())
    );

    // tests for badger_get_device_id
    test_extn_response!(
        test_badger_get_device_id_success,
        badger_get_device_id,
        ExtnResponse::Float(0.0),
        |resp: BadgerDialDeviceId| assert_eq!(resp.device_id, "dev123")
    );

    // tests for badger_network_connectivity
    test_extn_response!(
        test_badger_network_connectivity_success,
        badger_network_connectivity,
        ExtnResponse::Value(json!({
            "type": "ethernet",
            "state": "connected"
        })),
        |resp: BadgerNetworkConnectivity| assert_eq!(
            resp,
            BadgerNetworkConnectivity {
                network_interface: Some("ETHERNET".to_string()),
                status: BadgerNetworkConnectivityStatus::Success
            }
        )
    );

    // tests for badger_info
    test_with_cached_state_new!(
        test_badger_info_success,
        badger_info,
        ExtnResponse::BoolMap(HashMap::from([
            ("xrn:firebolt:capability:device:uid".to_string(), true),
            (
                "xrn:firebolt:capability:device:distributor".to_string(),
                true
            ),
            ("xrn:firebolt:capability:device:model".to_string(), true),
            ("xrn:firebolt:capability:profile:flags".to_string(), true),
            ("xrn:firebolt:capability:device:info".to_string(), true),
            ("xrn:firebolt:capability:network:status".to_string(), true),
            ("xrn:firebolt:capability:device:make".to_string(), true),
            ("xrn:firebolt:capability:device:sku".to_string(), true),
            ("xrn:firebolt:capability:device:id".to_string(), true),
            ("xrn:firebolt:capability:account:uid".to_string(), true),
            ("xrn:firebolt:capability:account:id".to_string(), true),
            (
                "xrn:firebolt:capability:localization:postal-code".to_string(),
                true
            ),
        ])),
        &(BadgerDeviceInfo{
                zip_code: Some("12345".to_string()),
                time_zone: Some("America/Los_Angeles".to_string()),
                time_zone_offset: Some("-7:00".to_string()),
                receiver_id: Some("dev123".to_string()),
                device_hash: Some("dev123".to_string()),
                device_id: Some("dev123".to_string()),
                account_id: Some("acc123".to_string()),
                device_capabilities: Some(BadgerDeviceCapabilities {
                    device_type: Some("ipstb".to_string()),
                    is_wifi_device: Some(true),
                    video_dimensions: Some(vec![1920, 1080]),
                    native_dimensions: Some(vec![1920, 1080]),
                    model: Some("SCXI11BEI".to_string()),
                    receiver_platform: Some("platform123".to_string()),
                    receiver_version: Some("version123".to_string()),
                    hdr: Some(BadgerHDR {
                        settop_hdr_support: vec![BadgerHdrProfile::Hdr10, BadgerHdrProfile::DolbyVision],
                        tv_hdr_support: vec![BadgerHdrProfile::Hdr10, BadgerHdrProfile::DolbyVision],
                    }),
                    hdcp: Some(BadgerHDCP {
                        supported_hdcp_version: "2.2".to_string(),
                        receiver_hdcp_version: "2.2".to_string(),
                        current_hdcp_version: "2.2".to_string(),
                        connected: true,
                        hdcp_compliant: true,
                        hdcp_enabled: true,
                        hdcp_reason: HDCP_REASON_RX_DEV_ON,
                    }),
                    web_browser: Some(BadgerWebBrowser {
                        user_agent: "Mozilla/5.0 (Linux; x86_64 GNU/Linux) AppleWebKit/601.1 (KHTML, like Gecko) Version/8.0 Safari/601.1 WPE".to_string(),
                        version: "1.0.0.0".to_string(),
                        browser_type: "WPE".to_string(),
                    }),
                    supports_true_sd: Some(true),
                    device_make_model: Some("SerComm_SCXI11BEI".to_string()),
                    audio_modes: Some(BadgerAudioModes {
                        current_audio_mode: vec![],
                        supported_audio_modes: vec!["dolbyAtmos".to_string(), "stereo".to_string()],
                    }),
                }),
                household_id: Some("acc123".to_string()),
                privacy_settings: HashMap::new(),
                partner_id: Some("sess123".to_string()),
                user_experience: Some("".to_string()),
            })
        );

    test_with_cached_state_new!(
        test_badger_info_partial_caps,
        badger_info,
        ExtnResponse::BoolMap(HashMap::from([
            ("xrn:firebolt:capability:device:uid".to_string(), true),
            ("xrn:firebolt:capability:device:model".to_string(), true),
            ("xrn:firebolt:capability:device:info".to_string(), true),
            ("xrn:firebolt:capability:network:status".to_string(), true),
            ("xrn:firebolt:capability:device:make".to_string(), true),
            ("xrn:firebolt:capability:device:sku".to_string(), true),
            ("xrn:firebolt:capability:device:id".to_string(), true),
            ("xrn:firebolt:capability:account:uid".to_string(), true),
            ("xrn:firebolt:capability:account:id".to_string(), true),
        ])),
        &(BadgerDeviceInfo{
                zip_code: None,
                time_zone: None,
                time_zone_offset: None,
                receiver_id: Some("dev123".to_string()),
                device_hash: Some("dev123".to_string()),
                device_id: Some("dev123".to_string()),
                account_id: Some("acc123".to_string()),
                device_capabilities: Some(BadgerDeviceCapabilities {
                    device_type: Some("ipstb".to_string()),
                    is_wifi_device: Some(true),
                    video_dimensions: Some(vec![1920, 1080]),
                    native_dimensions: Some(vec![1920, 1080]),
                    model: Some("SCXI11BEI".to_string()),
                    receiver_platform: Some("platform123".to_string()),
                    receiver_version: Some("version123".to_string()),
                    hdr: Some(BadgerHDR {
                        settop_hdr_support: vec![BadgerHdrProfile::Hdr10, BadgerHdrProfile::DolbyVision],
                        tv_hdr_support: vec![BadgerHdrProfile::Hdr10, BadgerHdrProfile::DolbyVision],
                    }),
                    hdcp: Some(BadgerHDCP {
                        supported_hdcp_version: "2.2".to_string(),
                        receiver_hdcp_version: "2.2".to_string(),
                        current_hdcp_version: "2.2".to_string(),
                        connected: true,
                        hdcp_compliant: true,
                        hdcp_enabled: true,
                        hdcp_reason: HDCP_REASON_RX_DEV_ON,
                    }),
                    web_browser: Some(BadgerWebBrowser {
                        user_agent: "Mozilla/5.0 (Linux; x86_64 GNU/Linux) AppleWebKit/601.1 (KHTML, like Gecko) Version/8.0 Safari/601.1 WPE".to_string(),
                        version: "1.0.0.0".to_string(),
                        browser_type: "WPE".to_string(),
                    }),
                    supports_true_sd: Some(true),
                    device_make_model: Some("SerComm_SCXI11BEI".to_string()),
                    audio_modes: Some(BadgerAudioModes {
                        current_audio_mode: vec![],
                        supported_audio_modes: vec!["dolbyAtmos".to_string(), "stereo".to_string()],
                    }),
                }),
                household_id: Some("acc123".to_string()),
                privacy_settings: HashMap::new(),
                partner_id: Some("sess123".to_string()),
                user_experience: Some("".to_string()),
            })
        );

    test_with_cached_state_new!(
        test_badger_info_minimal_caps,
        badger_info,
        ExtnResponse::BoolMap(HashMap::from([
            ("xrn:firebolt:capability:device:uid".to_string(), true),
            ("xrn:firebolt:capability:device:model".to_string(), true),
            ("xrn:firebolt:capability:device:info".to_string(), true),
        ])),
        &(BadgerDeviceInfo {
            zip_code: None,
            time_zone: None,
            time_zone_offset: None,
            receiver_id: Some("dev123".to_string()),
            device_hash: Some("dev123".to_string()),
            device_id: Some("dev123".to_string()),
            account_id: None,
            device_capabilities: Some(BadgerDeviceCapabilities {
                device_type: Some("ipstb".to_string()),
                is_wifi_device: None,
                video_dimensions: Some(vec![1920, 1080]),
                native_dimensions: Some(vec![1920, 1080]),
                model: Some("SCXI11BEI".to_string()),
                receiver_platform: None,
                receiver_version: None,
                hdr: None,
                hdcp: None,
                web_browser: None,
                supports_true_sd: None,
                device_make_model: Some("SerComm_SCXI11BEI".to_string()),
                audio_modes: None,
            }),
            household_id: None,
            privacy_settings: HashMap::new(),
            partner_id: None,
            user_experience: Some("".to_string()),
        })
    );

    // tests for badger_info
    test_with_cached_state_new!(
        test_badger_device_capabilities_success,
        device_capabilities,
        ExtnResponse::BoolMap(HashMap::from([
            ("xrn:firebolt:capability:device:info".to_string(), true),
            ("xrn:firebolt:capability:network:status".to_string(), true),
            ("xrn:firebolt:capability:device:make".to_string(), true),
            ("xrn:firebolt:capability:device:model".to_string(), true),
            ("xrn:firebolt:capability:device:sku".to_string(), true),
        ])),
        &(BadgerDeviceCapabilities {
                    device_type: Some("ipstb".to_string()),
                    is_wifi_device: Some(true),
                    video_dimensions: Some(vec![1920, 1080]),
                    native_dimensions: Some(vec![1920, 1080]),
                    model: Some("SCXI11BEI".to_string()),
                    receiver_platform: Some("platform123".to_string()),
                    receiver_version: Some("version123".to_string()),
                    hdr: Some(BadgerHDR {
                        settop_hdr_support: vec![BadgerHdrProfile::Hdr10, BadgerHdrProfile::DolbyVision],
                        tv_hdr_support: vec![BadgerHdrProfile::Hdr10, BadgerHdrProfile::DolbyVision],
                    }),
                    hdcp: Some(BadgerHDCP {
                        supported_hdcp_version: "2.2".to_string(),
                        receiver_hdcp_version: "2.2".to_string(),
                        current_hdcp_version: "2.2".to_string(),
                        connected: true,
                        hdcp_compliant: true,
                        hdcp_enabled: true,
                        hdcp_reason: HDCP_REASON_RX_DEV_ON,
                    }),
                    web_browser: Some(BadgerWebBrowser {
                        user_agent: "Mozilla/5.0 (Linux; x86_64 GNU/Linux) AppleWebKit/601.1 (KHTML, like Gecko) Version/8.0 Safari/601.1 WPE".to_string(),
                        version: "1.0.0.0".to_string(),
                        browser_type: "WPE".to_string(),
                    }),
                    supports_true_sd: Some(true),
                    device_make_model: Some("SerComm_SCXI11BEI".to_string()),
                    audio_modes: Some(BadgerAudioModes {
                        current_audio_mode: vec![],
                        supported_audio_modes: vec!["dolbyAtmos".to_string(), "stereo".to_string()],
                    }),
                })
    );

    test_with_cached_state_new!(
        test_badger_device_capabilities_partial_caps,
        device_capabilities,
        ExtnResponse::BoolMap(HashMap::from([
            ("xrn:firebolt:capability:device:info".to_string(), true),
            ("xrn:firebolt:capability:device:model".to_string(), true),
        ])),
        &(BadgerDeviceCapabilities {
            device_type: Some("ipstb".to_string()),
            is_wifi_device: None,
            video_dimensions: Some(vec![1920, 1080]),
            native_dimensions: Some(vec![1920, 1080]),
            model: Some("SCXI11BEI".to_string()),
            receiver_platform: None,
            receiver_version: None,
            hdr: None,
            hdcp: None,
            web_browser: None,
            supports_true_sd: None,
            device_make_model: Some("SerComm_SCXI11BEI".to_string()),
            audio_modes: None,
        })
    );

    test_with_cached_state_new!(
        test_badger_device_capabilities_min_caps,
        device_capabilities,
        ExtnResponse::BoolMap(HashMap::from([(
            "xrn:firebolt:capability:device:info".to_string(),
            true
        ),])),
        &(BadgerDeviceCapabilities {
            device_type: Some("ipstb".to_string()),
            is_wifi_device: None,
            video_dimensions: Some(vec![1920, 1080]),
            native_dimensions: Some(vec![1920, 1080]),
            model: None,
            receiver_platform: None,
            receiver_version: None,
            hdr: None,
            hdcp: None,
            web_browser: None,
            supports_true_sd: None,
            device_make_model: None,
            audio_modes: None,
        })
    );

    // tests for show_toaster_success
    test_extn_response!(
        test_show_toaster_success,
        show_toaster,
        ExtnResponse::None(()),
        |resp: DefaultResponse| assert_eq!(resp, DefaultResponse {})
    );

    // tests for badger_shutdown
    test_extn_response!(
        test_badger_shutdown_success,
        badger_shutdown,
        ExtnResponse::Value(Value::String("shutdown".to_string())),
        |resp: BadgerEmptyResult| assert_eq!(resp, BadgerEmptyResult::default())
    );

    // tests for badger_dismiss_loading_screen
    test_extn_response!(
        test_badger_dismiss_loading_screen_success,
        badger_dismiss_loading_screen,
        ExtnResponse::Value(Value::String("dismiss".to_string())),
        |resp: BadgerEmptyResult| assert_eq!(resp, BadgerEmptyResult::default())
    );

    // tests for badger_get_payload
    test_extn_response!(
        test_badger_get_payload_success,
        badger_get_payload,
        ExtnResponse::Value(json!(AppManagerResponse::SecondScreenPayload(
            "payload_data".to_string()
        ))),
        |resp: BadgerDialPayload| assert_eq!(resp.payload, Some("payload_data".to_string()))
    );

    // tests for badger_show_pin_overlay
    test_extn_response_with_params!(
        test_badger_show_pin_overlay_success,
        badger_show_pin_overlay,
        ExtnResponse::PinChallenge(PinChallengeResponse {
            granted: Some(true),
            reason: PinChallengeResultReason::CorrectPin,
        }),
        ShowPinOverlayRequest {
            pin_type: "purchase_pin".to_string(),
            suppress_snooze: None,
        },
        |resp: BadgerPinOverlayResponse| assert_eq!(
            resp,
            BadgerPinOverlayResponse {
                status: BadgerPinStatus::Success,
                message: "Successful Pin Validation".to_string(),
            }
        )
    );

    // tests for settings
    test_extn_response_with_params!(
        test_settings_success,
        settings,
        ExtnResponse::Settings(HashMap::from([(
            "VOICE_GUIDANCE_STATE".to_string(),
            SettingValue::bool(true),
        )])),
        BadgerSettingsRequest {
            keys: vec!["VOICE_GUIDANCE_STATE".to_string()],
        },
        |resp: HashMap<String, SettingValue>| assert_eq!(
            resp.get("VOICE_GUIDANCE_STATE").unwrap_or(&SettingValue {
                value: None,
                enabled: Some(true)
            }),
            &SettingValue {
                value: None,
                enabled: Some(true)
            }
        )
    );

    // tests for subscribe_to_settings
    test_extn_response_with_params!(
        test_subscribe_to_settings_success,
        subscribe_to_settings,
        ExtnResponse::None(()),
        BadgerSettingsRequest {
            keys: vec!["VOICE_GUIDANCE_STATE".to_string()],
        },
        |_| {}
    );

    // tests for prompt_email
    test_extn_response_with_params!(
        test_prompt_email_success,
        prompt_email,
        ExtnResponse::Keyboard(KeyboardSessionResponse {
            text: "test@example.com".to_string(),
            canceled: false,
        }),
        PromptEmailRequest {
            prefill_type: PrefillType::SignUp,
        },
        |resp: PromptEmailResult| assert_eq!(resp.data.email, "test@example.com".to_string())
    );

    // tests for badger_navigate_to_company_page
    test_extn_response_with_params!(
        test_badger_navigate_to_company_page_success,
        badger_navigate_to_company_page,
        ExtnResponse::DefaultApp(AppLibraryEntry {
            app_id: "app123".to_string(),
            manifest: AppManifestLoad::Remote("https://example.com".to_string()),
            boot_state: BootState::Inactive,
        }),
        NavigateCompanyPageRequest {
            company_id: "company123".to_string(),
        },
        |resp: BadgerEmptyResult| assert_eq!(resp, BadgerEmptyResult::default())
    );

    #[tokio::test(flavor = "multi_thread")]
    async fn test_get_account_session_cached() {
        let fixture = TestFixture::with_mocked_state();
        let result = get_account_session(&fixture.state).await;
        assert!(result.is_some());
        assert_eq!(result.unwrap().account_id, "acc123");
    }

    #[test]
    fn test_cached_state_new() {
        let state = BadgerState::mock();
        assert!(state.get_postal_code().is_none());
    }

    #[test]
    fn test_update_postal_code() {
        let state = BadgerState::mock();
        assert!(state.get_postal_code().is_none());

        state.update_postal_code(Some("12345".to_string()));
        assert_eq!(state.get_postal_code(), Some("12345".to_string()));
    }

    #[tokio::test(flavor = "multi_thread")]
    async fn test_get_is_wifi_device_success() {
        let fixture = TestFixture::new(true);
        fixture.queue_mock_extn_response(ExtnResponse::BoolMap(HashMap::from([(
            "xrn:firebolt:capability:network:wifi".to_string(),
            true,
        )])));
        let result = get_cached_is_wifi_device(
            &fixture.state,
            &mut fixture.state.get_client(),
            Some(fixture.request_id),
        )
        .await;
        assert!(result.is_some());
        assert!(result.unwrap());
    }
}
*/
