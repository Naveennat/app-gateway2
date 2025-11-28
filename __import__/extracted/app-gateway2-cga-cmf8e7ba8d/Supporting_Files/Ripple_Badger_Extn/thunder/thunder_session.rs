use thunder_ripple_sdk::{
    client::device_operator::{DeviceCallRequest, DeviceOperator},
    client::thunder_client::ThunderClient,
    ripple_sdk::{api::session::AccountSession, log::debug, utils::error::RippleError},
    thunder_state::ThunderState,
};

pub struct ThunderSessionService;

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

fn get_auth_method(method: &str) -> String {
    format!("org.rdk.AuthService.{}", method)
}

impl ThunderSessionService {
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
}
