use crate::util::http_client::{HttpClient, HttpError};
use serde::{Deserialize, Serialize};
use thunder_ripple_sdk::ripple_sdk::{
    api::firebolt::fb_discovery::{SessionParams, SignInRequestParams},
    log::error,
};
use url::{ParseError, Url};

pub struct XvpVideoService {}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
#[allow(dead_code)]
pub struct XvpVideoServiceErrResponse {
    #[serde(rename = "type")]
    pub _type: Option<String>,
    pub title: Option<String>,
    pub status: Option<i32>,
    pub detail: Option<String>,
    pub instance: Option<String>,
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
#[allow(dead_code)]
pub struct XvpVideoServiceResponse {
    pub partner_id: Option<String>,
    pub account_id: Option<String>,
    pub owner_reference: Option<String>,
    pub entity_urn: Option<String>,
    pub entity_id: Option<String>,
    pub entity_type: Option<String>,
    pub durable_app_id: Option<String>,
    pub event_type: Option<String>,
    pub is_signed_in: Option<bool>,
    pub added: Option<String>,
    pub updated: Option<String>,
}

#[derive(Debug, Serialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct VideoServiceSignInInfo {
    pub event_type: String,
    pub is_signed_in: bool,
}

impl XvpVideoService {
    pub fn build_session_url(
        base_url: &str,
        scope: &str,
        session_params: &SessionParams,
    ) -> Result<String, HttpError> {
        let mut session_url = Url::parse(base_url)?;

        // create owner_reference based on sign_in_state scope.
        let (subscriber_field_name, subscriber_value) = match scope {
            "device" => ("device", session_params.dist_session.device_id.clone()),
            _ => ("account", session_params.dist_session.account_id.clone()),
        };

        let owner_reference = format!(
            "{}{}{}{}",
            "xrn:xcal:subscriber:", subscriber_field_name, ":", subscriber_value,
        );
        let entity_urn = format!(
            "{}{}",
            "xrn:xvp:application:",
            session_params.app_id.clone()
        );

        session_url
            .path_segments_mut()
            .map_err(|_| ParseError::SetHostOnCannotBeABaseUrl)?
            .push("partners")
            .push(&session_params.dist_session.id)
            .push("accounts")
            .push(&session_params.dist_session.account_id)
            .push("videoServices")
            .push(&entity_urn)
            .push("engaged");

        session_url
            .query_pairs_mut()
            .append_pair("ownerReference", &owner_reference)
            .append_pair("clientId", "ripple");

        Ok(session_url.to_string())
    }

    pub async fn sign_in(
        base_url: &str,
        scope: &str,
        params: SignInRequestParams,
    ) -> Result<XvpVideoServiceResponse, HttpError> {
        let session_url =
            XvpVideoService::build_session_url(base_url, scope, &params.session_info)?;
        let sign_in_info = VideoServiceSignInInfo {
            event_type: "signIn".to_owned(),
            is_signed_in: params.is_signed_in,
        };
        let body = serde_json::to_string(&sign_in_info).unwrap();

        let mut http = HttpClient::new();
        let body_string = http
            .set_token(params.session_info.dist_session.token)
            .put(session_url, body)
            .await?;

        let result: Result<XvpVideoServiceResponse, serde_json::Error> =
            serde_json::from_str(&body_string);
        result.map_err(|e| {
            error!("XvpVideoServiceResponse parse error {:?}", e);
            HttpError::ServiceError
        })
    }
}
