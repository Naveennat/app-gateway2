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

use crate::badger_state::BadgerState;
use ripple_sdk::{
    api::{
        device::{
            device_info_request::{
                DeviceCapabilities, FirmwareInfo, DEVICE_INFO_AUTHORIZED,
                DEVICE_MAKE_MODEL_AUTHORIZED, DEVICE_SKU_AUTHORIZED,
            },
            device_request::{AudioProfile, HDCPStatus, HdrProfile, Resolution},
        },
        firebolt::fb_openrpc::FireboltSemanticVersion,
    },
    log::{error, info},
    serde_json::{self, Value},
    tokio::{join, sync::Mutex},
};
use serde::{Deserialize, Serialize};
use std::{collections::HashMap, sync::Arc};
use thunder_ripple_sdk::processors::thunder_device_info::ThunderDeviceInfoRequestProcessor;
use thunder_ripple_sdk::{
    client::{
        device_operator::{
            DeviceCallRequest, DeviceOperator, DeviceResponseMessage, DeviceSubscribeRequest,
        },
        thunder_plugin::ThunderPlugin,
    },
    utils::check_thunder_response_success,
};
use tokio::sync::mpsc;

#[derive(Debug, Serialize, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct SystemVersion {
    pub stb_version: String,
    pub receiver_version: String,
    pub stb_timestamp: String,
    pub success: bool,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct ThunderHDCPStatus {
    #[serde(rename = "HDCPStatus")]
    pub hdcp_status: HDCPStatus,
    pub success: bool,
}

pub mod hdr_flags {
    pub const HDRSTANDARD_NONE: u32 = 0x00;
    pub const HDRSTANDARD_HDR10: u32 = 0x01;
    pub const HDRSTANDARD_HLG: u32 = 0x02;
    pub const HDRSTANDARD_DOLBY_VISION: u32 = 0x04;
    pub const HDRSTANDARD_TECHNICOLOR_PRIME: u32 = 0x08;
    pub const HDRSTANDARD_HDR10PLUS: u32 = 0x10;
}

#[derive(Serialize, Clone, Debug, Deserialize)]
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
#[serde(rename_all = "camelCase")]
pub struct BadgerWebBrowser {
    pub(crate) user_agent: String,
    pub(crate) version: String,
    pub(crate) browser_type: String,
}

#[derive(Serialize, Clone, Debug, Deserialize, Default)]
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
}

#[derive(Serialize, Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct BadgerAudioModes {
    current_audio_mode: Vec<String>,
    supported_audio_modes: Vec<String>,
}

#[derive(Serialize, Clone, Debug)]
#[serde(rename_all = "camelCase")]
pub struct BadgerNetworkConnectivity {
    network_interface: Option<String>,
    status: BadgerNetworkConnectivityStatus,
}

#[derive(Serialize, Clone, Debug)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
pub enum BadgerNetworkConnectivityStatus {
    NoActiveNetworkInterface,
    Success,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct ThunderTimezoneResponse {
    #[serde(rename = "timeZone")]
    pub time_zone: String,
}

// Temporarily cache API responses to prevent redundant requests
lazy_static::lazy_static! {
    static ref CURRENT_RESOLUTION_CACHE: Arc<Mutex<Option<Vec<i32>>>> = Arc::new(Mutex::new(None));
    static ref SYSTEM_VERSIONS_CACHE: Arc<Mutex<Option<serde_json::Value>>> = Arc::new(Mutex::new(None));
}

pub(crate) async fn get_device_capabilities(
    state: &BadgerState,
    keys: &[&str],
) -> DeviceCapabilities {
    let device_info_authorized = keys.contains(&DEVICE_INFO_AUTHORIZED);
    let (
        video_dimensions,
        native_dimensions,
        firmware_info_result,
        hdr_info,
        hdcp_result,
        audio_result,
        model_result,
        make_result,
    ) = join!(
        async {
            if device_info_authorized {
                Some(get_cached_video_resolution(state).await)
            } else {
                None
            }
        },
        async {
            if device_info_authorized {
                Some(get_cached_screen_resolution(state).await)
            } else {
                None
            }
        },
        async {
            if device_info_authorized {
                Some(get_os_info(state).await.version)
            } else {
                None
            }
        },
        async {
            if device_info_authorized {
                Some(get_cached_hdr(state).await)
            } else {
                None
            }
        },
        async {
            if device_info_authorized {
                Some(get_cached_hdcp(state).await)
            } else {
                None
            }
        },
        async {
            if device_info_authorized {
                Some(get_cached_audio(state).await)
            } else {
                None
            }
        },
        async {
            if keys.contains(&DEVICE_SKU_AUTHORIZED) {
                Some(get_cached_model(state).await)
            } else {
                None
            }
        },
        async {
            if keys.contains(&DEVICE_MAKE_MODEL_AUTHORIZED) {
                Some(get_cached_make(state).await)
            } else {
                None
            }
        }
    );

    DeviceCapabilities {
        audio: audio_result,
        firmware_info: firmware_info_result,
        hdcp: hdcp_result,
        hdr: hdr_info,
        make: make_result,
        model: model_result,
        video_resolution: video_dimensions,
        screen_resolution: native_dimensions,
    }
}

async fn get_cached_screen_resolution(state: &BadgerState) -> Vec<i32> {
    if let Some(v) = state.get_native_dimensions() {
        v
    } else {
        get_screen_resolution(state).await
    }
}

async fn get_current_resolution_once(state: &BadgerState) -> Option<Vec<i32>> {
    let mut cache = CURRENT_RESOLUTION_CACHE.lock().await;

    // If already cached, return it
    if let Some(_response) = cache.as_ref() {
        return cache.clone();
    }

    // Make the API call
    let response = state
        .get_thunder_client()
        .call(DeviceCallRequest {
            method: ThunderPlugin::DisplaySettings.method("getCurrentResolution"),
            params: None,
        })
        .await;
    info!("getCurrentResolution response: {}", response.message);

    if check_thunder_response_success(&response) {
        let resol = response.message["resolution"].as_str().unwrap_or_default();
        let res = Some(get_dimension_from_resolution(resol));
        state.update_native_dimensions(res.clone());
        *cache = res.clone();
        res
    } else {
        *cache = None;
        None
    }
}

async fn get_screen_resolution(state: &BadgerState) -> Vec<i32> {
    let d = match get_current_resolution_once(state).await {
        Some(resolution) => resolution,
        None => {
            error!("Failed to get current resolution");
            state.update_account_session(None);
            return Vec::new();
        }
    };
    state.update_native_dimensions(Some(d.clone()));
    d
}

pub fn get_dimension_from_resolution(resolution: &str) -> Vec<i32> {
    match resolution {
        val if val.starts_with("480") => Resolution::Resolution480.dimension(),
        val if val.starts_with("576") => Resolution::Resolution576.dimension(),
        val if val.starts_with("540") => Resolution::Resolution540.dimension(),
        val if val.starts_with("720") => Resolution::Resolution720.dimension(),
        val if val.starts_with("1080") => Resolution::Resolution1080.dimension(),
        val if val.starts_with("2160") => Resolution::Resolution2160.dimension(),
        val if val.starts_with("4K") || val.starts_with("4k") => {
            Resolution::Resolution4k.dimension()
        }
        _ => Resolution::ResolutionDefault.dimension(),
    }
}

async fn get_default_resolution(state: &BadgerState) -> Result<Vec<i32>, ()> {
    let response = state
        .get_thunder_client()
        .call(DeviceCallRequest {
            method: ThunderPlugin::DisplaySettings.method("getDefaultResolution"),
            params: None,
        })
        .await;
    info!("getDefaultResolution response: {}", response.message);

    if !check_thunder_response_success(&response) {
        return Err(());
    }

    if let Some(resol) = response.message.get("defaultResolution") {
        if let Some(r) = resol.as_str() {
            return Ok(get_dimension_from_resolution(r));
        }
    }
    Err(())
}

async fn get_cached_video_resolution(state: &BadgerState) -> Vec<i32> {
    if let Some(v) = state.get_video_dimensions() {
        v
    } else {
        get_video_resolution(state).await
    }
}

async fn get_video_resolution(state: &BadgerState) -> Vec<i32> {
    match get_current_resolution_once(state).await {
        Some(resolution) => {
            state.update_video_dimensions(Some(resolution.clone()));
            return resolution;
        }
        None => {
            error!("Failed to get current resolution");
        }
    }

    if let Ok(resolution) = get_default_resolution(state).await {
        state.update_video_dimensions(Some(resolution.clone()));
        return resolution;
    }
    let dimensions = state
        .device_manifest
        .read()
        .unwrap()
        .configuration
        .default_values
        .video_dimensions
        .clone();
    state.update_video_dimensions(Some(dimensions.clone()));
    dimensions
}

async fn get_system_versions_once(state: &BadgerState) -> Option<serde_json::Value> {
    let mut cache = SYSTEM_VERSIONS_CACHE.lock().await;

    // If already cached, return it
    if let Some(response) = cache.as_ref() {
        return Some(response.clone());
    }

    // Make the API call
    let response = state
        .get_thunder_client()
        .call(DeviceCallRequest {
            method: ThunderPlugin::System.method("getSystemVersions"),
            params: None,
        })
        .await;
    info!("getSystemVersions response: {}", response.message);

    if check_thunder_response_success(&response) {
        *cache = Some(response.message.clone());
        Some(response.message)
    } else {
        *cache = None;
        None
    }
}

async fn get_os_info(state: &BadgerState) -> FirmwareInfo {
    let version: FireboltSemanticVersion;
    // TODO: refactor this to use return syntax and not use response variable across branches
    match state.get_version() {
        Some(v) => version = v,
        None => {
            let ver = match get_system_versions_once(state).await {
                Some(ver) => ver,
                None => {
                    error!("Failed to get system versions");
                    serde_json::Value::Null
                }
            };

            if let Ok(tsv) = serde_json::from_value::<SystemVersion>(ver) {
                let tsv_split = tsv.receiver_version.split('.');
                let tsv_vec: Vec<&str> = tsv_split.collect();

                if tsv_vec.len() >= 3 {
                    let major: String = tsv_vec[0].chars().filter(|c| c.is_ascii_digit()).collect();
                    let minor: String = tsv_vec[1].chars().filter(|c| c.is_ascii_digit()).collect();
                    let patch: String = tsv_vec[2].chars().filter(|c| c.is_ascii_digit()).collect();

                    version = FireboltSemanticVersion {
                        major: major.parse::<u32>().unwrap(),
                        minor: minor.parse::<u32>().unwrap(),
                        patch: patch.parse::<u32>().unwrap(),
                        readable: tsv.stb_version,
                    };
                    state.update_version(Some(version.clone()));
                } else {
                    version = FireboltSemanticVersion {
                        readable: tsv.stb_version,
                        ..FireboltSemanticVersion::default()
                    };
                    state.update_version(Some(version.clone()));
                }
            } else {
                error!("Failed to get system versions");
                state.update_version(None);
                version = FireboltSemanticVersion::default();
            }
        }
    }
    FirmwareInfo {
        name: "rdk".into(),
        version,
    }
}

async fn get_cached_make(state: &BadgerState) -> String {
    if let Some(v) = state.get_make() {
        v
    } else {
        get_make(state).await
    }
}

async fn get_make(state: &BadgerState) -> String {
    let resp = state
        .get_thunder_client()
        .call(DeviceCallRequest {
            method: ThunderPlugin::System.method("getDeviceInfo"),
            params: None,
        })
        .await;
    info!("getDeviceInfo response: {}", resp.message);

    let r = resp.message.get("make");
    if r.is_none() {
        error!("device make is not available");
        state.update_make(None);
        "".into()
    } else {
        let make = r.unwrap().as_str().unwrap().trim_matches('"');
        state.update_make(Some(make.to_string().clone()));
        make.to_string()
    }
}

async fn get_cached_model(state: &BadgerState) -> String {
    if let Some(v) = state.get_model() {
        v
    } else {
        get_model(state).await
    }
}

async fn get_model(state: &BadgerState) -> String {
    let resp = match get_system_versions_once(state).await {
        Some(v) => v,
        None => {
            error!("Failed to get system versions");
            state.update_model(None);
            return "NA".to_owned();
        }
    };

    let model_str = resp
        .get("stbVersion")
        .and_then(|v| v.as_str())
        .map(|s| s.trim_matches('"'))
        .unwrap_or("");

    if model_str.is_empty() {
        error!("device model is not available");
        state.update_model(None);
        return "NA".to_owned();
    }

    let model = model_str.split('_').next().unwrap_or("").to_owned();
    state.update_model(Some(model.clone()));
    model
}

async fn get_cached_hdr(state: &BadgerState) -> HashMap<HdrProfile, bool> {
    if let Some(v) = state.get_hdr_profile() {
        v
    } else {
        get_hdr(state).await
    }
}

async fn get_hdr(state: &BadgerState) -> HashMap<HdrProfile, bool> {
    let response = state
        .get_thunder_client()
        .call(DeviceCallRequest {
            method: ThunderPlugin::DisplaySettings.method("getTVHDRCapabilities"),
            params: None,
        })
        .await;
    info!("getTVHDRCapabilities response: {}", response.message);

    if !check_thunder_response_success(&response) {
        state.update_hdr_profile(None);
        return HashMap::new();
    }

    let supported_cap: u32 = response.message["capabilities"]
        .to_string()
        .parse()
        .unwrap_or(0);
    let mut hm = HashMap::new();
    hm.insert(
        HdrProfile::Hdr10,
        0 != (supported_cap & hdr_flags::HDRSTANDARD_HDR10),
    );
    hm.insert(
        HdrProfile::Hlg,
        0 != (supported_cap & hdr_flags::HDRSTANDARD_HLG),
    );
    hm.insert(
        HdrProfile::DolbyVision,
        0 != (supported_cap & hdr_flags::HDRSTANDARD_DOLBY_VISION),
    );
    hm.insert(
        HdrProfile::Technicolor,
        0 != (supported_cap & hdr_flags::HDRSTANDARD_TECHNICOLOR_PRIME),
    );
    hm.insert(
        HdrProfile::Hdr10plus,
        0 != (supported_cap & hdr_flags::HDRSTANDARD_HDR10PLUS),
    );
    state.update_hdr_profile(Some(hm.clone()));
    hm
}

async fn get_cached_hdcp(state: &BadgerState) -> HDCPStatus {
    if let Some(v) = state.get_hdcp_status() {
        v
    } else {
        get_hdcp_status(state).await
    }
}

async fn get_hdcp_status(state: &BadgerState) -> HDCPStatus {
    let mut response: HDCPStatus = HDCPStatus::default();
    let resp = state
        .get_thunder_client()
        .call(DeviceCallRequest {
            method: ThunderPlugin::Hdcp.method("getHDCPStatus"),
            params: None,
        })
        .await;
    info!("getHDCPStatus response: {}", resp.message);

    if let Ok(thdcp) = serde_json::from_value::<ThunderHDCPStatus>(resp.message) {
        response = thdcp.hdcp_status;
        state.update_hdcp_status(Some(response.clone()));
    } else {
        error!("Failed to get HDCP status");
        state.update_hdcp_status(None);
    }
    response
}

async fn get_cached_audio(state: &BadgerState) -> HashMap<AudioProfile, bool> {
    if let Some(v) = state.get_audio_profile() {
        v
    } else {
        get_audio(state).await
    }
}

async fn get_audio(state: &BadgerState) -> HashMap<AudioProfile, bool> {
    let response = state
        .get_thunder_client()
        .call(DeviceCallRequest {
            method: ThunderPlugin::DisplaySettings.method("getAudioFormat"),
            params: None,
        })
        .await;
    info!("getAudioFormat response: {}", response.message);

    if !check_thunder_response_success(&response) {
        state.update_audio_profile(None);
        return HashMap::new();
    }

    let audio = get_audio_profile_from_value(response.message);
    state.update_audio_profile(Some(audio.clone()));
    audio
}

pub fn get_audio_profile_from_value(value: Value) -> HashMap<AudioProfile, bool> {
    let mut hm: HashMap<AudioProfile, bool> = HashMap::new();
    hm.insert(AudioProfile::Stereo, false);
    hm.insert(AudioProfile::DolbyDigital5_1, false);
    hm.insert(AudioProfile::DolbyDigital5_1Plus, false);
    hm.insert(AudioProfile::DolbyDigital7_1, false);
    hm.insert(AudioProfile::DolbyDigital7_1Plus, false);
    hm.insert(AudioProfile::DolbyAtmos, false);

    let supported_profiles = match value.get("supportedAudioFormat") {
        Some(profiles) => profiles.as_array().unwrap(),
        None => return hm,
    };

    for profile in supported_profiles {
        let profile_name = profile.as_str().unwrap();
        match profile_name {
            "PCM" => {
                hm.insert(AudioProfile::Stereo, true);
                hm.insert(AudioProfile::DolbyDigital5_1, true);
            }
            "DOLBY AC3" => {
                hm.insert(AudioProfile::Stereo, true);
                hm.insert(AudioProfile::DolbyDigital5_1, true);
            }
            "DOLBY EAC3" => {
                hm.insert(AudioProfile::Stereo, true);
                hm.insert(AudioProfile::DolbyDigital5_1, true);
            }
            "DOLBY AC4" => {
                hm.insert(AudioProfile::Stereo, true);
                hm.insert(AudioProfile::DolbyDigital5_1, true);
                hm.insert(AudioProfile::DolbyDigital7_1, true);
            }
            "DOLBY TRUEHD" => {
                hm.insert(AudioProfile::Stereo, true);
                hm.insert(AudioProfile::DolbyDigital5_1, true);
                hm.insert(AudioProfile::DolbyDigital7_1, true);
            }
            "DOLBY EAC3 ATMOS" => {
                hm.insert(AudioProfile::Stereo, true);
                hm.insert(AudioProfile::DolbyDigital5_1, true);
                hm.insert(AudioProfile::DolbyDigital7_1, true);
                hm.insert(AudioProfile::DolbyAtmos, true);
            }
            "DOLBY TRUEHD ATMOS" => {
                hm.insert(AudioProfile::Stereo, true);
                hm.insert(AudioProfile::DolbyDigital5_1, true);
                hm.insert(AudioProfile::DolbyDigital7_1, true);
                hm.insert(AudioProfile::DolbyAtmos, true);
            }
            "DOLBY AC4 ATMOS" => {
                hm.insert(AudioProfile::Stereo, true);
                hm.insert(AudioProfile::DolbyDigital5_1, true);
                hm.insert(AudioProfile::DolbyDigital7_1, true);
                hm.insert(AudioProfile::DolbyAtmos, true);
            }
            _ => (),
        }
    }
    hm
}

pub(crate) async fn subscribe_to_device_changes(cached_state: &BadgerState) {
    // Get Timezone and offset on boot and have listeners for changes
    ThunderDeviceInfoRequestProcessor::get_timezone_and_offset(&cached_state.state).await;

    let events = vec![
        (
            ThunderPlugin::System.callsign_and_version(),
            "onTimeZoneDSTChanged",
        ),
        (
            ThunderPlugin::DisplaySettings.callsign_and_version(),
            "audioFormatChanged",
        ),
        (
            ThunderPlugin::HdcpProfile.callsign_and_version(),
            "onDisplayConnectionChanged",
        ),
        (
            ThunderPlugin::DisplaySettings.callsign_and_version(),
            "resolutionChanged",
        ),
        (
            ThunderPlugin::Network.callsign_and_version(),
            "onConnectionStatusChanged",
        ),
    ];

    for (module, event_name) in events {
        let (sub_tx, mut sub_rx) = mpsc::channel::<DeviceResponseMessage>(5);
        let client = cached_state.get_state().get_thunder_client();
        let request = DeviceSubscribeRequest {
            module,
            event_name: event_name.into(),
            params: None,
            sub_id: None,
        };

        if let Err(e) = client.subscribe(request, sub_tx).await {
            error!("Failed to subscribe to {}: {:?}", event_name, e);
        } else {
            info!("Successfully subscribed to {}", event_name);
        }

        let cached_state_clone = cached_state.clone();

        tokio::spawn(async move {
            while let Some(message) = sub_rx.recv().await {
                // Check if the message contains a new time zone (onTimeZoneDSTChanged)
                if message.message.get("newTimeZone").is_some() {
                    ThunderDeviceInfoRequestProcessor::get_timezone_and_offset(
                        &cached_state_clone.state,
                    )
                    .await;

                // Check if the message contains a resolution change (resolutionChanged)
                } else if message.message.get("resolution").is_some() {
                    get_video_resolution(&cached_state_clone).await;
                    get_screen_resolution(&cached_state_clone).await;

                // Check if the message contains a status update (onInternetStatusChange)
                } else if message.message.get("interface").is_some() {
                    let is_wifi_device =
                        matches!(message.message["interface"].as_str(), Some("WIFI"));
                    cached_state_clone.update_is_wifi_device(Some(is_wifi_device));

                // Check if the message contains an HDCP status update (onDisplayConnectionChanged)
                } else if message.message.get("HDCPStatus").is_some() {
                    get_hdcp_status(&cached_state_clone).await;
                    get_hdr(&cached_state_clone).await;

                // Check if the message contains an audio format update (audioFormatChanged)
                } else if message.message.get("currentAudioFormat").is_some() {
                    get_audio(&cached_state_clone).await;
                } else {
                    error!(
                        "Received unknown message from thunder event notification: {:?}",
                        message.message
                    );
                }
            }
        });
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    use crate::test_utils::test_utils::TestFixture;

    #[tokio::test]
    async fn test_get_dimension_from_resolution() {
        assert_eq!(
            get_dimension_from_resolution("480p"),
            vec![720, 480],
            "failed to get dimensions for 480p"
        );
        assert_eq!(
            get_dimension_from_resolution("720p"),
            vec![1280, 720],
            "failed to get dimensions for 720p"
        );
        assert_eq!(
            get_dimension_from_resolution("1080p"),
            vec![1920, 1080],
            "failed to get dimensions for 1080p"
        );
        assert_eq!(
            get_dimension_from_resolution("2160p"),
            vec![3840, 2160],
            "failed to get dimensions for 2160p"
        );
        assert_eq!(
            get_dimension_from_resolution("4K"),
            vec![3840, 2160],
            "failed to get dimensions for 4K"
        );
        assert_eq!(
            get_dimension_from_resolution("0"),
            vec![1920, 1080],
            "failed to get dimensions for default resolution"
        );
    }

    #[tokio::test]
    async fn test_get_audio_profile_from_value() {
        let value = serde_json::json!({
            "supportedAudioFormat": ["PCM", "DOLBY AC3", "DOLBY EAC3", "DOLBY AC4", "DOLBY TRUEHD", "DOLBY EAC3 ATMOS", "DOLBY TRUEHD ATMOS", "DOLBY AC4 ATMOS"]
        });
        let audio_profiles = get_audio_profile_from_value(value);
        assert!(
            audio_profiles[&AudioProfile::Stereo],
            "Stereo should be true"
        );
        assert!(
            audio_profiles[&AudioProfile::DolbyDigital5_1],
            "DolbyDigital5_1 should be true"
        );
        assert!(
            audio_profiles[&AudioProfile::DolbyDigital7_1],
            "DolbyDigital7_1 should be true"
        );
        assert!(
            audio_profiles[&AudioProfile::DolbyAtmos],
            "DolbyAtmos should be true"
        );
        assert!(
            !audio_profiles[&AudioProfile::DolbyDigital5_1Plus],
            "DolbyDigital5_1Plus should be false by default"
        );
        assert!(
            !audio_profiles[&AudioProfile::DolbyDigital7_1Plus],
            "DolbyDigital7_1Plus should be false by default"
        );
    }

    #[tokio::test]
    async fn test_get_cached_screen_resolution() {
        let fixture = TestFixture::with_mocked_state();
        let resolution = get_cached_screen_resolution(&fixture.state).await;
        assert_eq!(resolution, vec![1920, 1080]);
    }

    #[tokio::test]
    async fn test_get_cached_video_resolution() {
        let fixture = TestFixture::with_mocked_state();
        let resolution = get_cached_video_resolution(&fixture.state).await;
        assert_eq!(resolution, vec![1920, 1080]);
    }

    #[tokio::test]
    async fn test_get_cached_make() {
        let fixture = TestFixture::with_mocked_state();
        let make = get_cached_make(&fixture.state).await;
        assert_eq!(make, "SerComm");
    }

    #[tokio::test]
    async fn test_get_cached_model() {
        let fixture = TestFixture::with_mocked_state();
        let model = get_cached_model(&fixture.state).await;
        assert_eq!(model, "SCXI11BEI");
    }

    #[tokio::test]
    async fn test_get_cached_hdr() {
        let fixture = TestFixture::with_mocked_state();
        let hdr = get_cached_hdr(&fixture.state).await;
        assert!(hdr[&HdrProfile::Hdr10]);
        assert!(hdr[&HdrProfile::DolbyVision]);
    }

    #[tokio::test]
    async fn test_get_cached_hdcp() {
        let fixture = TestFixture::with_mocked_state();
        let hdcp = get_cached_hdcp(&fixture.state).await;
        assert_eq!(hdcp.supported_hdcp_version, "2.2");
    }

    #[tokio::test]
    async fn test_get_cached_audio() {
        let fixture = TestFixture::with_mocked_state();
        let audio = get_cached_audio(&fixture.state).await;
        assert!(audio[&AudioProfile::Stereo]);
        assert!(audio[&AudioProfile::DolbyAtmos]);
    }

    #[tokio::test]
    async fn test_get_dimension_from_resolution_invalid() {
        assert_eq!(
            get_dimension_from_resolution("invalid"),
            vec![1920, 1080],
            "Expected default resolution for invalid input"
        );
    }

    #[tokio::test]
    async fn test_get_audio_profile_from_value_empty() {
        let value = serde_json::json!({});
        let audio_profiles = get_audio_profile_from_value(value);
        assert!(
            !audio_profiles[&AudioProfile::Stereo],
            "Stereo should be false by default"
        );
        assert!(
            !audio_profiles[&AudioProfile::DolbyDigital5_1],
            "DolbyDigital5_1 should be false by default"
        );
    }
}
