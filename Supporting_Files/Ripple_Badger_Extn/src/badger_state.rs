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
use crate::handlers::badger_info_rpc::BadgerWebBrowser;
use ripple_sdk::service::service_client::ServiceClient;
use ripple_sdk::{
    api::{
        device::device_request::{AudioProfile, HDCPStatus, HdrProfile},
        firebolt::fb_openrpc::FireboltSemanticVersion,
        manifest::device_manifest::{DefaultValues, DeviceManifest},
        session::AccountSession,
    },
    extn::client::extn_client::ExtnClient,
};
use std::collections::HashMap;
use std::sync::{Arc, RwLock};
use thunder_ripple_sdk::{client::thunder_client::ThunderClient, thunder_state::ThunderState};

pub(crate) const DEFAULT_USER_AGENT: &str = "Mozilla/5.0 (Linux; x86_64 GNU/Linux) AppleWebKit/601.1 (KHTML, like Gecko) Version/8.0 Safari/601.1 WPE";
pub(crate) const DEFAULT_VERSION: &str = "1.0.0.0";
pub(crate) const DEFAULT_BROWSER_TYPE: &str = "WPE";

#[derive(Debug, Clone, Default)]
pub(crate) struct CachedBadgerInfo {
    pub(crate) postal_code: Option<String>,
    pub(crate) account_session: Option<AccountSession>,
    pub(crate) device_uid: Option<HashMap<String, String>>,
    pub(crate) account_uid: Option<HashMap<String, String>>,
    pub(crate) version: Option<FireboltSemanticVersion>,
    pub(crate) receiver_platform: Option<String>,
    pub(crate) receiver_version: Option<String>,
    pub(crate) web_browser: Option<BadgerWebBrowser>,
    pub(crate) device_make: Option<String>,
    pub(crate) device_model: Option<String>,
    pub(crate) device_make_model: Option<String>,
    pub(crate) hdcp_status: Option<HDCPStatus>,
    pub(crate) hdr_profile: Option<HashMap<HdrProfile, bool>>,
    pub(crate) video_dimensions: Option<Vec<i32>>,
    pub(crate) native_dimensions: Option<Vec<i32>>,
    pub(crate) audio_profile: Option<HashMap<AudioProfile, bool>>,
    pub(crate) is_wifi_device: Option<bool>,
}

#[derive(Debug, Clone)]
pub(crate) struct BadgerState {
    pub(crate) state: Arc<ThunderState>,
    pub(crate) cached: Arc<RwLock<CachedBadgerInfo>>,
    pub(crate) service_client: ServiceClient,
    pub(crate) device_manifest: Arc<RwLock<DeviceManifest>>,
}

impl BadgerState {
    pub fn new(
        state: ThunderState,
        device_manifest: DeviceManifest,
        service_client: ServiceClient,
    ) -> Self {
        Self {
            state: Arc::new(state),
            cached: Arc::new(RwLock::new(CachedBadgerInfo::default())),
            device_manifest: Arc::new(RwLock::new(device_manifest)),
            service_client,
        }
    }

    pub(crate) fn get_client(&self) -> ExtnClient {
        self.state.get_client()
    }

    pub(crate) fn get_service_client(&self) -> ServiceClient {
        self.service_client.clone()
    }

    pub(crate) fn get_thunder_client(&self) -> ThunderClient {
        self.state.get_thunder_client()
    }

    #[allow(dead_code)]
    pub(crate) fn get_state(&self) -> &ThunderState {
        &self.state
    }

    #[allow(dead_code)]
    pub(crate) fn get_cached(&self) -> Arc<RwLock<CachedBadgerInfo>> {
        self.cached.clone()
    }

    pub(crate) fn get_default(&self) -> DefaultValues {
        self.device_manifest
            .read()
            .unwrap()
            .configuration
            .default_values
            .clone()
    }

    pub(crate) fn get_postal_code(&self) -> Option<String> {
        self.cached.read().unwrap().postal_code.clone()
    }

    pub(crate) fn update_postal_code(&self, postal_code: Option<String>) {
        let mut cached = self.cached.write().unwrap();
        cached.postal_code = postal_code;
    }

    pub(crate) fn get_distributor_experience_id(&self) -> String {
        self.device_manifest
            .read()
            .unwrap()
            .configuration
            .distributor_experience_id
            .clone()
    }

    pub(crate) fn get_account_session(&self) -> Option<AccountSession> {
        self.cached.read().unwrap().account_session.clone()
    }

    pub(crate) fn update_account_session(&self, account_session: Option<AccountSession>) {
        let mut cached = self.cached.write().unwrap();
        cached.account_session = account_session;
    }

    pub(crate) fn get_device_uid(&self) -> Option<HashMap<String, String>> {
        self.cached.read().unwrap().device_uid.clone()
    }

    pub(crate) fn update_device_uid(&self, device_uid: Option<HashMap<String, String>>) {
        let mut cached = self.cached.write().unwrap();
        cached.device_uid = device_uid;
    }

    pub(crate) fn get_account_uid(&self) -> Option<HashMap<String, String>> {
        self.cached.read().unwrap().account_uid.clone()
    }

    pub(crate) fn update_account_uid(&self, account_uid: Option<HashMap<String, String>>) {
        let mut cached = self.cached.write().unwrap();
        cached.account_uid = account_uid;
    }

    pub(crate) fn get_make(&self) -> Option<String> {
        self.cached.read().unwrap().device_make.clone()
    }

    pub(crate) fn update_make(&self, device_make: Option<String>) {
        let mut cached = self.cached.write().unwrap();
        cached.device_make = device_make;
    }

    pub(crate) fn get_model(&self) -> Option<String> {
        self.cached.read().unwrap().device_model.clone()
    }

    pub(crate) fn update_model(&self, device_model: Option<String>) {
        let mut cached = self.cached.write().unwrap();
        cached.device_model = device_model;
    }

    pub(crate) fn get_make_model(&self) -> Option<String> {
        self.cached.read().unwrap().device_make_model.clone()
    }

    pub(crate) fn update_make_model(&self, make_model: Option<String>) {
        let mut cached = self.cached.write().unwrap();
        cached.device_make_model = make_model;
    }

    pub(crate) fn get_version(&self) -> Option<FireboltSemanticVersion> {
        self.cached.read().unwrap().version.clone()
    }

    pub(crate) fn update_version(&self, version: Option<FireboltSemanticVersion>) {
        let mut cached = self.cached.write().unwrap();
        cached.version = version;
    }

    pub(crate) fn get_receiver_platform(&self) -> Option<String> {
        self.cached.read().unwrap().receiver_platform.clone()
    }

    pub(crate) fn update_receiver_platform(&self, receiver_platform: Option<String>) {
        let mut cached = self.cached.write().unwrap();
        cached.receiver_platform = receiver_platform;
    }

    pub(crate) fn get_receiver_version(&self) -> Option<String> {
        self.cached.read().unwrap().receiver_version.clone()
    }

    pub(crate) fn update_receiver_version(&self, receiver_version: Option<String>) {
        let mut cached = self.cached.write().unwrap();
        cached.receiver_version = receiver_version;
    }

    pub(crate) fn get_web_browser(&self) -> Option<BadgerWebBrowser> {
        let mut cached = self.cached.write().unwrap();
        if cached.web_browser.is_none() {
            let web_browser = BadgerWebBrowser {
                user_agent: DEFAULT_USER_AGENT.to_string(),
                version: DEFAULT_VERSION.to_string(),
                browser_type: DEFAULT_BROWSER_TYPE.to_string(),
            };
            cached.web_browser = Some(web_browser.clone());
            Some(web_browser)
        } else {
            cached.web_browser.clone()
        }
    }

    #[allow(dead_code)]
    fn update_web_browser(&self, web_browser: Option<BadgerWebBrowser>) {
        let mut cached = self.cached.write().unwrap();
        cached.web_browser = web_browser;
    }

    pub(crate) fn get_hdcp_status(&self) -> Option<HDCPStatus> {
        self.cached.read().unwrap().hdcp_status.clone()
    }

    pub(crate) fn update_hdcp_status(&self, hdcp_status: Option<HDCPStatus>) {
        let mut cached = self.cached.write().unwrap();
        cached.hdcp_status = hdcp_status;
    }

    pub(crate) fn get_hdr_profile(&self) -> Option<HashMap<HdrProfile, bool>> {
        self.cached.read().unwrap().hdr_profile.clone()
    }

    pub(crate) fn update_hdr_profile(&self, hdr_profile: Option<HashMap<HdrProfile, bool>>) {
        let mut cached = self.cached.write().unwrap();
        cached.hdr_profile = hdr_profile;
    }

    pub(crate) fn get_video_dimensions(&self) -> Option<Vec<i32>> {
        self.cached.read().unwrap().video_dimensions.clone()
    }

    pub(crate) fn update_video_dimensions(&self, video_dimensions: Option<Vec<i32>>) {
        let mut cached = self.cached.write().unwrap();
        cached.video_dimensions = video_dimensions;
    }

    pub(crate) fn get_native_dimensions(&self) -> Option<Vec<i32>> {
        self.cached.read().unwrap().native_dimensions.clone()
    }

    pub(crate) fn update_native_dimensions(&self, native_dimensions: Option<Vec<i32>>) {
        let mut cached = self.cached.write().unwrap();
        cached.native_dimensions = native_dimensions;
    }

    pub(crate) fn get_audio_profile(&self) -> Option<HashMap<AudioProfile, bool>> {
        self.cached.read().unwrap().audio_profile.clone()
    }

    pub(crate) fn update_audio_profile(&self, audio_profile: Option<HashMap<AudioProfile, bool>>) {
        let mut cached = self.cached.write().unwrap();
        cached.audio_profile = audio_profile;
    }

    pub(crate) fn get_is_wifi_device(&self) -> Option<bool> {
        self.cached.read().unwrap().is_wifi_device
    }

    pub(crate) fn update_is_wifi_device(&self, is_wifi_device: Option<bool>) {
        {
            let mut cached = self.cached.write().unwrap();
            cached.is_wifi_device = is_wifi_device;
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::test_utils::test_utils::mock_thunder_state;
    use ripple_sdk::api::device::device_request::{AudioProfile, HDCPStatus, HdrProfile};
    use ripple_sdk::api::session::AccountSession;
    use ripple_sdk::service::service_client::ServiceClient;
    use ripple_sdk::Mockable;

    impl Mockable for BadgerState {
        fn mock() -> Self {
            let state = mock_thunder_state();
            let device_manifest = DeviceManifest::default();
            let service_client = ServiceClient::default();
            BadgerState::new(state, device_manifest, service_client)
        }
    }

    #[test]
    fn test_update_postal_code() {
        let cached_state = BadgerState::mock();
        cached_state.update_postal_code(Some("12345".to_string()));
        assert_eq!(cached_state.get_postal_code(), Some("12345".to_string()));
    }

    #[test]
    fn test_update_account_session() {
        let cached_state = BadgerState::mock();
        let account_session = AccountSession::default();
        cached_state.update_account_session(Some(account_session.clone()));
        assert_eq!(cached_state.get_account_session(), Some(account_session));
    }

    #[test]
    fn test_update_device_uid() {
        let cached_state = BadgerState::mock();
        let mut device_uid = HashMap::new();
        device_uid.insert("some_app".to_string(), "device_uid_123".to_string());
        cached_state.update_device_uid(Some(device_uid.clone()));
        assert_eq!(cached_state.get_device_uid(), Some(device_uid));
    }

    #[test]
    fn test_update_hdcp_status() {
        let cached_state = BadgerState::mock();
        let hdcp_status = HDCPStatus::default();
        cached_state.update_hdcp_status(Some(hdcp_status.clone()));
        assert_eq!(cached_state.get_hdcp_status(), Some(hdcp_status));
    }

    #[test]
    fn test_update_hdr_profile() {
        let cached_state = BadgerState::mock();
        let mut hdr_profile = HashMap::new();
        hdr_profile.insert(HdrProfile::Hdr10, true);
        cached_state.update_hdr_profile(Some(hdr_profile.clone()));
        assert_eq!(cached_state.get_hdr_profile(), Some(hdr_profile));
    }

    #[test]
    fn test_update_audio_profile() {
        let cached_state = BadgerState::mock();
        let mut audio_profile = HashMap::new();
        audio_profile.insert(AudioProfile::Stereo, true);
        cached_state.update_audio_profile(Some(audio_profile.clone()));
        assert_eq!(cached_state.get_audio_profile(), Some(audio_profile));
    }

    #[test]
    fn test_update_is_wifi_device() {
        let cached_state = BadgerState::mock();
        cached_state.update_is_wifi_device(Some(true));
        assert_eq!(cached_state.get_is_wifi_device(), Some(true));
    }

    #[test]
    fn test_get_distributor_experience_id() {
        let cached_state = BadgerState::mock();
        assert_eq!(cached_state.get_distributor_experience_id(), "".to_string());
    }

    #[test]
    fn test_update_model() {
        let cached_state = BadgerState::mock();
        cached_state.update_model(Some("device_model_123".to_string()));
        assert_eq!(
            cached_state.get_model(),
            Some("device_model_123".to_string())
        );
    }

    #[test]
    fn test_update_make_model() {
        let cached_state = BadgerState::mock();
        cached_state.update_make_model(Some("device_make_model_123".to_string()));
        assert_eq!(
            cached_state.get_make_model(),
            Some("device_make_model_123".to_string())
        );
    }

    #[test]
    fn test_update_version() {
        let cached_state = BadgerState::mock();
        let version = FireboltSemanticVersion::default();
        cached_state.update_version(Some(version.clone()));
        assert_eq!(cached_state.get_version(), Some(version));
    }

    #[test]
    fn test_update_receiver_platform() {
        let cached_state = BadgerState::mock();
        cached_state.update_receiver_platform(Some("receiver_platform_123".to_string()));
        assert_eq!(
            cached_state.get_receiver_platform(),
            Some("receiver_platform_123".to_string())
        );
    }

    #[test]
    fn test_update_receiver_version() {
        let cached_state = BadgerState::mock();
        cached_state.update_receiver_version(Some("receiver_version_123".to_string()));
        assert_eq!(
            cached_state.get_receiver_version(),
            Some("receiver_version_123".to_string())
        );
    }

    #[test]
    fn test_update_video_dimensions() {
        let cached_state = BadgerState::mock();
        let video_dimensions = vec![1920, 1080];
        cached_state.update_video_dimensions(Some(video_dimensions.clone()));
        assert_eq!(cached_state.get_video_dimensions(), Some(video_dimensions));
    }

    #[test]
    fn test_update_native_dimensions() {
        let cached_state = BadgerState::mock();
        let native_dimensions = vec![1280, 720];
        cached_state.update_native_dimensions(Some(native_dimensions.clone()));
        assert_eq!(
            cached_state.get_native_dimensions(),
            Some(native_dimensions)
        );
    }
}
