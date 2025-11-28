use crate::util::http_client::{HttpClient, HttpError};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use thunder_ripple_sdk::ripple_sdk::{
    api::{
        firebolt::fb_advertising::AdIdRequestParams, firebolt::fb_discovery::SessionParams,
        session::AccountSession,
    },
    log::error,
};
use url::{ParseError, Url};

pub enum Advertising {
    AcquireAdId(AdIdRequestParams),
    ResetAdId(SessionParams), // no lmt & entityScopeId in request
}

pub struct XvpXifaService {}

#[derive(Debug, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
#[allow(dead_code)]
pub struct XvpXifaServiceErrResponse {
    #[serde(rename = "type")]
    pub _type: Option<String>,
    pub title: Option<String>,
    pub status: Option<i32>,
    pub detail: Option<String>,
    pub instance: Option<String>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
#[allow(dead_code)]
pub struct XvpXifaServiceResponse {
    pub partner_id: Option<String>,
    pub account_id: Option<String>,
    pub device_id: Option<String>,
    pub profile_id: Option<String>,
    pub xifa: String,
    pub xifa_type: String,
    pub entity_scope_ids: Option<Vec<String>>,
    pub match_rule: Option<MatchRule>,
    pub expiration: Option<String>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct MatchRule {
    pub match_rule_id: Option<String>,
    pub match_rule_version: Option<i32>,
}

pub enum XifaOperation {
    Get,
    Reset,
}

pub struct XifaParams {
    pub privacy_data: HashMap<String, String>,
    pub app_id: Option<String>,
    pub dist_session: AccountSession,
    pub scope: HashMap<String, String>,
}

impl XvpXifaService {
    pub fn build_xifa_url(
        base_url: &str,
        params: &XifaParams,
        operation: XifaOperation,
    ) -> Result<String, HttpError> {
        let mut xifa_url = Url::parse(&base_url)?;

        xifa_url
            .path_segments_mut()
            .map_err(|_| ParseError::SetHostOnCannotBeABaseUrl)?
            .push("partners")
            .push(&params.dist_session.id)
            .push("accounts")
            .push(&params.dist_session.account_id);

        match operation {
            XifaOperation::Get => {
                xifa_url
                    .path_segments_mut()
                    .unwrap()
                    .push("devices")
                    .push(&params.dist_session.device_id)
                    .push("autoresolve")
                    .push("xifa");
                let ad_tracking_participation_state = params
                    .privacy_data
                    .get("lmt")
                    .map_or(false, |lmt| lmt == "0");

                let query_string = format!(
                    "adTrackingParticipationState={}",
                    ad_tracking_participation_state
                );

                xifa_url.set_query(Some(&query_string));
            }
            XifaOperation::Reset => {
                xifa_url
                    .path_segments_mut()
                    .unwrap()
                    .push("xifas")
                    .push("reset");
            }
        }

        xifa_url.query_pairs_mut().append_pair("clientId", "ripple");

        if let Some(app_id) = &params.app_id {
            let mut entity_urn = format!("xrn:xvp:application:{}", app_id);
            if !&params.scope.is_empty()
                && params.scope.get("type").is_some()
                && params.scope.get("id").is_some()
            {
                entity_urn = format!(
                    "xrn:xvp:application:{}:{}:{}",
                    app_id,
                    params.scope.get("type").unwrap(),
                    params.scope.get("id").unwrap()
                );
            }
            xifa_url
                .query_pairs_mut()
                .append_pair("entityScopeId", &entity_urn);
        }

        Ok(xifa_url.to_string())
    }

    pub async fn get_ad_identifier(
        base_url: &str,
        params: XifaParams,
    ) -> Result<XvpXifaServiceResponse, HttpError> {
        let xifa_url = XvpXifaService::build_xifa_url(base_url, &params, XifaOperation::Get)?;

        let mut http = HttpClient::new();
        let body_string = http
            .set_token(params.dist_session.token)
            .get(xifa_url, String::new())
            .await?;
        let result: Result<XvpXifaServiceResponse, serde_json::Error> =
            serde_json::from_str(&body_string);
        result.map_err(|e| {
            error!("XvpXifaServiceResponse parse error {:?}", e);
            HttpError::ServiceError
        })
    }

    pub async fn reset_ad_identifier(
        base_url: &str,
        params: XifaParams,
    ) -> Result<Vec<XvpXifaServiceResponse>, HttpError> {
        let xifa_url = XvpXifaService::build_xifa_url(base_url, &params, XifaOperation::Reset)?;

        let mut http = HttpClient::new();
        let body_string = http
            .set_token(params.dist_session.token)
            .post(xifa_url, String::new())
            .await?;
        let result: Result<Vec<XvpXifaServiceResponse>, serde_json::Error> =
            serde_json::from_str(&body_string);
        result.map_err(|e| {
            error!("XvpXifaServiceResponse parse error {:?}", e);
            HttpError::ServiceError
        })
    }
}
