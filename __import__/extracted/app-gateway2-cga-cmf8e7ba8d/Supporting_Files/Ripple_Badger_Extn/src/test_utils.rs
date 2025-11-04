#[cfg(any(test, feature = "mock"))]
pub(crate) mod test_utils {
    use crate::badger_state::{
        BadgerState, CachedBadgerInfo, DEFAULT_BROWSER_TYPE, DEFAULT_USER_AGENT, DEFAULT_VERSION,
    };
    use crate::handlers::badger_info_rpc::BadgerWebBrowser;
    use ripple_sdk::api::context::RippleContextUpdateRequest;
    use ripple_sdk::api::device::device_request::TimeZone;
    use ripple_sdk::api::manifest::device_manifest::DeviceManifest;
    use ripple_sdk::api::manifest::extn_manifest::ExtnSymbol;
    use ripple_sdk::extn::extn_id::ExtnClassId;
    use ripple_sdk::service::service_client::ServiceClient;
    use ripple_sdk::Mockable;
    use ripple_sdk::{
        api::{
            device::device_request::{AudioProfile, HDCPStatus, HdrProfile},
            firebolt::fb_openrpc::FireboltSemanticVersion,
            gateway::rpc_gateway_api::CallContext,
            session::AccountSession,
        },
        chrono::Utc,
        extn::{
            client::extn_client::ExtnClient,
            extn_client_message::{ExtnMessage, ExtnPayload, ExtnResponse},
            extn_id::ExtnId,
        },
        framework::ripple_contract::RippleContract,
        utils::error::RippleError,
    };

    use ripple_sdk::utils::mock_utils::queue_mock_response;

    use std::collections::HashMap;
    use std::sync::{Arc, RwLock};
    use thunder_ripple_sdk::{client::thunder_client::ThunderClient, thunder_state::ThunderState};

    pub(crate) fn mock_extn_client(need_magic: bool) -> ExtnClient {
        let mut config = HashMap::new();
        if need_magic {
            config.insert("magic".to_string(), "123abc".to_string());
        }
        let (client, _) = ExtnClient::new_extn(ExtnSymbol {
            id: format!(
                "{}",
                ExtnId::new_channel(ExtnClassId::Device, "ripple".to_string())
            ),
            uses: vec![],
            fulfills: vec![],
            config: Some(config),
        });
        let tz = TimeZone {
            time_zone: "America/Los_Angeles".to_string(),
            offset: -25200,
        };
        client.context_update(RippleContextUpdateRequest::TimeZone(tz));
        client
    }

    pub(crate) fn mock_thunder_client() -> ThunderClient {
        ThunderClient::mock()
    }

    pub(crate) fn mock_thunder_state() -> ThunderState {
        let ext_client = mock_extn_client(true);
        let thunder_client = mock_thunder_client();
        ThunderState::new(ext_client, thunder_client)
    }

    pub(crate) fn get_mocked_cached_state() -> BadgerState {
        let mut ap = HashMap::new();
        ap.insert(AudioProfile::DolbyAtmos, true);
        ap.insert(AudioProfile::Stereo, true);

        BadgerState {
            cached: Arc::new(RwLock::new(CachedBadgerInfo {
                account_session: Some(AccountSession {
                    account_id: "acc123".to_string(),
                    device_id: "dev123".to_string(),
                    id: "sess123".to_string(),
                    token: "some_token".to_string(),
                }),
                postal_code: Some("12345".to_string()),
                device_uid: Some(HashMap::from([(
                    "internal".to_string(),
                    "dev123".to_string(),
                )])),
                account_uid: Some(HashMap::from([(
                    "internal".to_string(),
                    "acc123".to_string(),
                )])),
                receiver_platform: Some("platform123".to_string()),
                receiver_version: Some("version123".to_string()),
                web_browser: Some(BadgerWebBrowser {
                    user_agent: DEFAULT_USER_AGENT.to_string(),
                    version: DEFAULT_VERSION.to_string(),
                    browser_type: DEFAULT_BROWSER_TYPE.to_string(),
                }),
                device_model: Some("SCXI11BEI".to_string()),
                device_make_model: Some("SerComm_SCXI11BEI".to_string()),
                version: Some(FireboltSemanticVersion {
                    major: 1,
                    minor: 0,
                    patch: 0,
                    readable: "some".to_string(),
                }),
                device_make: Some("SerComm".to_string()),
                hdcp_status: Some(HDCPStatus {
                    is_connected: true,
                    is_hdcp_compliant: true,
                    is_hdcp_enabled: true,
                    supported_hdcp_version: "2.2".to_string(),
                    receiver_hdcp_version: "2.2".to_string(),
                    current_hdcp_version: "2.2".to_string(),
                    hdcp_reason: 1,
                }),
                hdr_profile: Some(HashMap::from([
                    (HdrProfile::Hdr10, true),
                    (HdrProfile::DolbyVision, true),
                ])),
                video_dimensions: Some(vec![1920, 1080]),
                native_dimensions: Some(vec![1920, 1080]),
                audio_profile: Some(ap),
                is_wifi_device: Some(true),
            })),
            state: mock_thunder_state().into(),
            device_manifest: Arc::new(RwLock::new(DeviceManifest::default())),
            service_client: ServiceClient::default(),
        }
    }

    #[allow(dead_code)]
    #[derive(Debug)]
    pub(crate) struct TestFixture {
        pub state: BadgerState,
        pub ctx: CallContext,
        pub request_id: String,
    }

    #[allow(dead_code)]
    impl TestFixture {
        pub(crate) fn new(_need_magic: bool) -> Self {
            let ctx = CallContext::internal("test.method");
            let request_id = ctx.request_id.clone();
            let state = BadgerState::mock();

            Self {
                state,
                ctx,
                request_id,
            }
        }

        pub(crate) fn only_session_mock() -> Self {
            let ctx = CallContext::internal("test.method");
            let request_id = ctx.request_id.clone();
            let mut state = BadgerState::mock();

            // Mock only the account session
            state.cached = Arc::new(RwLock::new(CachedBadgerInfo {
                account_session: Some(AccountSession {
                    account_id: "acc123".to_string(),
                    device_id: "dev123".to_string(),
                    id: "sess123".to_string(),
                    token: "some_token".to_string(),
                }),
                ..Default::default() // Use default values for other fields
            }));

            Self {
                state,
                ctx,
                request_id,
            }
        }

        pub(crate) fn with_mocked_state() -> Self {
            let ctx = CallContext::internal("test.method");
            let request_id = ctx.request_id.clone();
            let state = get_mocked_cached_state();
            state
                .device_manifest
                .write()
                .unwrap()
                .configuration
                .form_factor = "ipstb".into();
            Self {
                state,
                ctx,
                request_id,
            }
        }

        pub(crate) fn queue_mock_extn_response(&self, payload: ExtnResponse) {
            let msg = ExtnMessage {
                id: "test_id".to_string(),
                requestor: ExtnId::get_main_target("main".into()),
                target: RippleContract::Internal,
                target_id: Some(ExtnId::get_main_target("main".into())),
                payload: ExtnPayload::Response(payload),
                ts: Some(Utc::now().timestamp_millis()),
            };
            queue_mock_response(&self.request_id, Ok(msg));
        }

        #[allow(dead_code)]
        pub(crate) fn queue_parse_error(&self) {
            queue_mock_response(&self.request_id, Err(RippleError::ParseError));
        }

        #[allow(dead_code)]
        pub(crate) fn queue_timeout_error(&self) {
            queue_mock_response(&self.request_id, Err(RippleError::TimeoutError));
        }

        pub(crate) fn queue_error_resp(&self, err: RippleError) {
            queue_mock_response(&self.request_id, Err(err));
        }
    }
}
