use serde::{Deserialize, Serialize};
use thunder_ripple_sdk::{
    client::device_operator::{DeviceCallRequest, DeviceChannelParams, DeviceOperator},
    client::thunder_client::{DefaultThunderResult, ThunderClient},
    events::thunder_event_processor::ThunderEventHandlerProvider,
    ripple_sdk::{
        api::{
            device::{device_events::DeviceEventCallback, device_request::AccountToken},
            session::{
                AccountSession, AccountSessionRequest, AccountSessionResponse,
                AccountSessionTokenRequest, ProvisionRequest, SessionAdjective,
            },
        },
        async_trait::async_trait,
        extn::{
            client::{
                extn_client::ExtnClient,
                extn_processor::{
                    DefaultExtnStreamer, ExtnRequestProcessor, ExtnStreamProcessor, ExtnStreamer,
                },
            },
            extn_client_message::{ExtnMessage, ExtnResponse},
        },
        framework::ripple_contract::RippleContract,
        log::{debug, error},
        serde_json,
        tokio::sync::mpsc,
        utils::error::RippleError,
    },
    thunder_state::ThunderState,
};

use super::events::comcast_thunder_event_handlers::SessionTokenChangeHandler;

#[derive(Debug, Serialize, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct SetSatThunderRequest {
    pub token: String,
    pub status: u32,
    pub expires: u32,
}

impl SetSatThunderRequest {
    #[allow(dead_code)]
    fn get(a_t_r: AccountSessionTokenRequest) -> SetSatThunderRequest {
        SetSatThunderRequest {
            status: 0,
            token: a_t_r.token,
            expires: a_t_r.expires_in,
        }
    }
}

pub struct ThunderSessionService {
    state: ThunderState,
    streamer: DefaultExtnStreamer,
}

async fn thunder_getter(
    client: ThunderClient,
    method: String,
    prop: &'static str,
    default: &'static str,
) -> String {
    let request = DeviceCallRequest {
        method,
        params: None,
    };
    let response = client.call(request).await;
    debug!("thunder_getter_response={:?}", response);
    match response.message.get(prop) {
        Some(val) => match val.as_str() {
            Some(s) => String::from(s),
            None => String::from(default),
        },
        None => String::from(default),
    }
}

#[allow(dead_code)]
async fn thunder_setter(client: ThunderClient, method: String, key: String, value: String) -> bool {
    let request = DeviceCallRequest {
        method,
        params: Some(DeviceChannelParams::Json(format!(
            r#"
{{
"{}":"{}"
}}
"#,
            key, value
        ))),
    };

    let response = client.call(request).await;
    if let Ok(result) = serde_json::from_value::<DefaultThunderResult>(response.message) {
        return result.success;
    }

    false
}

fn get_auth_method(method: &str) -> String {
    format!("org.rdk.AuthService.{}", method)
}

impl ThunderSessionService {
    pub fn new(state: ThunderState) -> ThunderSessionService {
        ThunderSessionService {
            state,
            streamer: DefaultExtnStreamer::new(),
        }
    }

    pub async fn get_sat_token(state: &ThunderState) -> String {
        let m_sat = get_auth_method("getServiceAccessToken");
        thunder_getter(state.get_thunder_client(), m_sat, "token", "").await
    }

    async fn get_session_token(state: ThunderState, request: ExtnMessage) -> bool {
        let method = get_auth_method("getServiceAccessToken");
        let dev_request = DeviceCallRequest {
            method,
            params: None,
        };
        let client = state.get_thunder_client();
        let dev_response = client.call(dev_request).await;
        if let Ok(device_token) =
            serde_json::from_value::<AccountToken>(dev_response.message.clone())
        {
            Self::respond(
                state.get_client(),
                request,
                ExtnResponse::AccountSession(AccountSessionResponse::AccountSessionToken(
                    device_token,
                )),
            )
            .await
            .is_ok()
        } else {
            Self::handle_error(state.get_client(), request, RippleError::ExtnError).await
        }
    }

    pub async fn get_account_session(state: &ThunderState) -> Result<AccountSession, RippleError> {
        let m_did = get_auth_method("getXDeviceId");
        let did = thunder_getter(state.get_thunder_client(), m_did, "xDeviceId", "").await;
        let m_aid = get_auth_method("getServiceAccountId");
        let aid = thunder_getter(state.get_thunder_client(), m_aid, "serviceAccountId", "").await;
        let m_pid = get_auth_method("getDeviceId");
        let pid = thunder_getter(state.get_thunder_client(), m_pid, "partnerId", "").await;
        let m_sat = get_auth_method("getServiceAccessToken");
        let sat = thunder_getter(state.get_thunder_client(), m_sat, "token", "").await;

        if !pid.is_empty() && !sat.is_empty() && !aid.is_empty() && !did.is_empty() {
            Ok(AccountSession {
                id: pid,
                token: sat,
                account_id: aid,
                device_id: did,
            })
        } else {
            Err(RippleError::InvalidOutput)
        }
    }

    async fn get_session(state: ThunderState, request: ExtnMessage) -> bool {
        if let Ok(r) = Self::get_account_session(&state).await {
            Self::respond(
                state.get_client(),
                request,
                ExtnResponse::AccountSession(AccountSessionResponse::AccountSession(r)),
            )
            .await
            .is_ok()
        } else {
            Self::handle_error(state.get_client(), request, RippleError::ExtnError).await
        }
    }

    pub(crate) async fn set_provision(state: ThunderState, session: ProvisionRequest) -> bool {
        if session.distributor_id.is_none() {
            error!("set_provision: session.distributor_id is not set, cannot set provisioning");
            false;
        }
        thunder_setter(
            state.get_thunder_client(),
            get_auth_method("setServiceAccountId"),
            String::from("serviceAccountId"),
            session.account_id.clone(),
        )
        .await
            && thunder_setter(
                state.get_thunder_client(),
                get_auth_method("setXDeviceId"),
                String::from("xDeviceId"),
                session.device_id.clone(),
            )
            .await
            && thunder_setter(
                state.get_thunder_client(),
                get_auth_method("setPartnerId"),
                String::from("partnerId"),
                session.distributor_id.unwrap(),
            )
            .await
    }

    pub(crate) async fn set_session(
        state: ThunderState,
        a_t_r: AccountSessionTokenRequest,
    ) -> bool {
        let set_sat_thunder_request = SetSatThunderRequest::get(a_t_r);
        let device_request = DeviceCallRequest {
            method: get_auth_method("setServiceAccessToken"),
            params: Some(DeviceChannelParams::Json(
                serde_json::to_string(&set_sat_thunder_request).unwrap(),
            )),
        };
        let sat_response = state.get_thunder_client().call(device_request).await;
        if let Ok(result) = serde_json::from_value::<DefaultThunderResult>(sat_response.message) {
            if result.success {
                return true;
            }
        }
        false
    }

    pub async fn setup_session_handler(state: &ThunderState) {
        state
            .handle_listener(
                true,
                "internal".to_owned(),
                SessionTokenChangeHandler::provide(
                    "internal".to_owned(),
                    DeviceEventCallback::ExtnEvent,
                ),
            )
            .await;
    }

    async fn subscribe(state: ThunderState, request: ExtnMessage) -> bool {
        Self::setup_session_handler(&state).await;

        Self::ack(state.get_client(), request).await.is_ok()
    }
}

impl ExtnStreamProcessor for ThunderSessionService {
    type STATE = ThunderState;
    type VALUE = AccountSessionRequest;

    fn get_state(&self) -> Self::STATE {
        self.state.clone()
    }

    fn receiver(&mut self) -> mpsc::Receiver<ExtnMessage> {
        self.streamer.receiver()
    }

    fn sender(&self) -> mpsc::Sender<ExtnMessage> {
        self.streamer.sender()
    }

    fn contract(&self) -> RippleContract {
        RippleContract::Session(SessionAdjective::Account)
    }
}

#[async_trait]
impl ExtnRequestProcessor for ThunderSessionService {
    fn get_client(&self) -> ExtnClient {
        self.state.get_client()
    }
    async fn process_request(
        state: Self::STATE,
        msg: ExtnMessage,
        extracted_message: Self::VALUE,
    ) -> bool {
        match extracted_message {
            AccountSessionRequest::Get => Self::get_session(state.clone(), msg).await,
            AccountSessionRequest::GetAccessToken => {
                Self::get_session_token(state.clone(), msg).await
            }
            AccountSessionRequest::Subscribe => Self::subscribe(state.clone(), msg).await,
        }
    }
}
#[cfg(test)]
pub mod contract_tests {
    use ripple_eos_test_utils::{thunder_client, thunder_state};

    use super::*;
    ripple_eos_test_utils::pact_prelude!();
    /*
    AccountSessionRequest::Get
    */
    #[tokio::test(flavor = "multi_thread")]
    #[cfg_attr(not(feature = "websocket_contract_tests"), ignore)]
    pub async fn test_thunder_get_session_xdeviceid() {
        ripple_eos_test_utils::test_logging_setup!();
        ripple_eos_test_utils::mock_websocket_server!(
            builder,
            server,
            server_url,
            "thunder_session_get_xdevice_id",
            json!({
                "pact:content-type": "application/json",
                "request": {"jsonrpc": "matching(type, '2.0')", "id": "matching(integer, 0)", "method":  get_auth_method("getXDeviceId")},
                "requestMetadata": {
                    "path": "/jsonrpc"
                },
                "response": [{
                    "jsonrpc": "matching(type, '2.0')",
                    "id": "matching(integer, 0)",
                    "result": {
                        "xDeviceId": "foo",
                        "status": "0"
                    }
                }]
            })
        );
        assert!(
            thunder_getter(
                thunder_client!(server_url),
                get_auth_method("getXDeviceId"),
                "xDeviceId",
                ""
            )
            .await
                == "foo"
        );
    }

    #[tokio::test(flavor = "multi_thread")]
    #[cfg_attr(not(feature = "websocket_contract_tests"), ignore)]
    pub async fn test_thunder_get_session_service_account_id() {
        ripple_eos_test_utils::test_logging_setup!();
        ripple_eos_test_utils::mock_websocket_server!(
            builder,
            server,
            server_url,
            "thunder_session_get_service_account_id",
            json!({
                "pact:content-type": "application/json",
                "request": {"jsonrpc": "matching(type, '2.0')", "id": "matching(integer, 0)", "method":  get_auth_method("getServiceAccountId")},
                "requestMetadata": {
                    "path": "/jsonrpc"
                },
                "response": [{
                    "jsonrpc": "matching(type, '2.0')",
                    "id": "matching(integer, 0)",
                    "result": {
                        "serviceAccountId": "foo",
                        "status": "0"
                    }
                }]
            })
        );
        assert!(
            thunder_getter(
                thunder_client!(server_url),
                get_auth_method("getServiceAccountId"),
                "serviceAccountId",
                ""
            )
            .await
                == "foo"
        );
    }

    #[tokio::test(flavor = "multi_thread")]
    #[cfg_attr(not(feature = "websocket_contract_tests"), ignore)]
    pub async fn test_thunder_get_session_partner_id() {
        ripple_eos_test_utils::test_logging_setup!();
        ripple_eos_test_utils::mock_websocket_server!(
            builder,
            server,
            server_url,
            "thunder_session_get_partner_id",
            json!({
                "pact:content-type": "application/json",
                "request": {"jsonrpc": "matching(type, '2.0')", "id": "matching(integer, 0)", "method":  get_auth_method("getDeviceId")},
                "requestMetadata": {
                    "path": "/jsonrpc"
                },
                "response": [{
                    "jsonrpc": "matching(type, '2.0')",
                    "id": "matching(integer, 0)",
                    "result": {
                        "partnerId": "xumo",
                        "status": "0"
                    }
                }]
            })
        );
        assert!(
            thunder_getter(
                thunder_client!(server_url),
                get_auth_method("getDeviceId"),
                "partnerId",
                ""
            )
            .await
                == "xumo"
        );
    }
    #[tokio::test(flavor = "multi_thread")]
    #[cfg_attr(not(feature = "websocket_contract_tests"), ignore)]
    pub async fn test_thunder_get_service_access_token() {
        ripple_eos_test_utils::test_logging_setup!();
        ripple_eos_test_utils::mock_websocket_server!(
            builder,
            server,
            server_url,
            "thunder_session_get_partner_id",
            json!({
                "pact:content-type": "application/json",
                "request": {"jsonrpc": "matching(type, '2.0')", "id": "matching(integer, 0)", "method":  get_auth_method("getServiceAccessToken")},
                "requestMetadata": {
                    "path": "/jsonrpc"
                },
                "response": [{
                    "jsonrpc": "matching(type, '2.0')",
                    "id": "matching(integer, 0)",
                    "result": {
                        "token": "sleep",
                        "status": "0"
                    }
                }]
            })
        );
        assert!(
            thunder_getter(
                thunder_client!(server_url),
                get_auth_method("getServiceAccessToken"),
                "token",
                ""
            )
            .await
                == "sleep"
        );
    }

    #[tokio::test(flavor = "multi_thread")]
    #[cfg_attr(not(feature = "websocket_contract_tests"), ignore)]
    pub async fn test_thunder_set_partner_id() {
        ripple_eos_test_utils::test_logging_setup!();
        ripple_eos_test_utils::mock_websocket_server!(
            builder,
            server,
            server_url,
            "thunder_session_set_partner_id",
            json!({
                "pact:content-type": "application/json",
                "request": {"jsonrpc": "matching(type, '2.0')", "id": "matching(integer, 0)", "method":  get_auth_method("setPartnerId")},
                "requestMetadata": {
                    "path": "/jsonrpc"
                },
                "response": [{
                    "jsonrpc": "matching(type, '2.0')",
                    "id": "matching(integer, 0)",
                    "result": {
                        "success": true
                    }
                }]
            })
        );

        assert!(
            thunder_setter(
                thunder_client!(server_url),
                get_auth_method("setPartnerId"),
                "partnerId".into(),
                "xumo".into()
            )
            .await
        );
    }

    #[tokio::test(flavor = "multi_thread")]
    #[cfg_attr(not(feature = "websocket_contract_tests"), ignore)]
    pub async fn test_thunder_set_xdevice_id() {
        ripple_eos_test_utils::test_logging_setup!();
        ripple_eos_test_utils::mock_websocket_server!(
            builder,
            server,
            server_url,
            "thunder_session_set_xdevice_id",
            json!({
                "pact:content-type": "application/json",
                "request": {"jsonrpc": "matching(type, '2.0')", "id": "matching(integer, 0)", "method":  get_auth_method("setXDeviceId")},
                "requestMetadata": {
                    "path": "/jsonrpc"
                },
                "response": [{
                    "jsonrpc": "matching(type, '2.0')",
                    "id": "matching(integer, 0)",
                    "result": {
                        "success": true
                    }
                }]
            })
        );

        assert!(
            thunder_setter(
                thunder_client!(server_url),
                get_auth_method("setXDeviceId"),
                "partnerId".into(),
                "xumo".into()
            )
            .await
        );
    }

    #[tokio::test(flavor = "multi_thread")]
    #[cfg_attr(not(feature = "websocket_contract_tests"), ignore)]
    pub async fn test_thunder_set_service_account_id() {
        ripple_eos_test_utils::test_logging_setup!();
        ripple_eos_test_utils::mock_websocket_server!(
            builder,
            server,
            server_url,
            "thunder_session_set_service_account_id",
            json!({
                "pact:content-type": "application/json",
                "request": {"jsonrpc": "matching(type, '2.0')", "id": "matching(integer, 0)", "method":  get_auth_method("setServiceAccountId")},
                "requestMetadata": {
                    "path": "/jsonrpc"
                },
                "response": [{
                    "jsonrpc": "matching(type, '2.0')",
                    "id": "matching(integer, 0)",
                    "result": {
                        "success": true
                    }
                }]
            })
        );

        assert!(
            thunder_setter(
                thunder_client!(server_url),
                get_auth_method("setServiceAccountId"),
                "partnerId".into(),
                "xumo".into()
            )
            .await
        );
    }

    #[tokio::test(flavor = "multi_thread")]
    #[cfg_attr(not(feature = "websocket_contract_tests"), ignore)]
    pub async fn test_thunder_set_service_access_token() {
        ripple_eos_test_utils::test_logging_setup!();
        ripple_eos_test_utils::mock_websocket_server!(
            builder,
            server,
            server_url,
            "thunder_session_set_service_access_token",
            json!({
                "pact:content-type": "application/json",
                "request": {"jsonrpc": "matching(type, '2.0')", "id": "matching(integer, 0)", "method":  get_auth_method("setServiceAccessToken")},
                "requestMetadata": {
                    "path": "/jsonrpc"
                },
                "response": [{
                    "jsonrpc": "matching(type, '2.0')",
                    "id": "matching(integer, 0)",
                    "result": {
                        "success": true
                    }
                }]
            })
        );

        assert!(
            ThunderSessionService::set_session(
                thunder_state!(server_url),
                AccountSessionTokenRequest {
                    token: "token".into(),
                    expires_in: 11
                }
            )
            .await
        );
    }
}
