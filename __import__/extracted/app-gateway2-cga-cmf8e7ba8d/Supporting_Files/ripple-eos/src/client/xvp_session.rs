use crate::util::http_client::{HttpClient, HttpError};
use serde::Deserialize;
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_discovery::{
    ClearContentSetParams, ContentAccessInfo, ContentAccessListSetParams, SessionParams,
};
use url::{ParseError, Url};

pub struct XvpSession {}

#[derive(Debug, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
#[allow(dead_code)]
struct XvpSessioErrorResponse {
    #[serde(rename = "type")]
    _type: Option<String>,
    title: Option<String>,
    status: Option<i32>,
    detail: Option<String>,
    instance: Option<String>,
}

impl XvpSession {
    pub fn build_session_url(
        base_url: String,
        session_params: &SessionParams,
    ) -> Result<String, HttpError> {
        let mut session_url = Url::parse(&base_url)?;

        session_url
            .path_segments_mut()
            .map_err(|_| ParseError::SetHostOnCannotBeABaseUrl)?
            .push("partners")
            .push(&session_params.dist_session.id)
            .push("accounts")
            .push(&session_params.dist_session.account_id)
            .push("appSettings")
            .push(&session_params.app_id);

        session_url
            .query_pairs_mut()
            .append_pair("deviceId", &session_params.dist_session.device_id)
            .append_pair("clientId", "ripple");

        Ok(session_url.to_string())
    }

    pub async fn set_content_access(
        base_url: String,
        params: ContentAccessListSetParams,
    ) -> Result<(), HttpError> {
        let session_url = XvpSession::build_session_url(base_url, &params.session_info)?;
        let body = serde_json::to_string(&params.content_access_info).unwrap();

        let mut http = HttpClient::new();
        let _body_string = http
            .set_token(params.session_info.dist_session.token.clone())
            .put(session_url, body)
            .await?;
        Ok(())
    }

    pub async fn clear_content_access(
        base_url: String,
        params: ClearContentSetParams,
    ) -> Result<(), HttpError> {
        let session_url = XvpSession::build_session_url(base_url, &params.session_info)?;

        // pass empty vectors for clearing the contents
        let content_access_info = ContentAccessInfo {
            availabilities: Some(vec![]),
            entitlements: Some(vec![]),
        };
        let body = serde_json::to_string(&content_access_info).unwrap();

        let mut http = HttpClient::new();
        let _body_string = http
            .set_token(params.session_info.dist_session.token.clone())
            .put(session_url, body)
            .await?;
        Ok(())
    }
}
