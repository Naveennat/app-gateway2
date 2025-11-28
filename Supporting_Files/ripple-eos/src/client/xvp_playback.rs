use serde::{Deserialize, Serialize};
use std::collections::HashSet;
use url::{ParseError, Url};

use crate::util::http_client::{HttpClient, HttpError};
use thunder_ripple_sdk::ripple_sdk::{
    api::firebolt::fb_discovery::{MediaEvent, MediaEventsAccountLinkRequestParams, ProgressUnit},
    log::{debug, error},
};

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
struct ResumePointBody {
    durable_app_id: String,
    progress: f32,
    progress_units: ProgressUnit,
    completed: bool,
    cet: HashSet<String>,
    #[serde(rename = "cet_NotPropagated")]
    cet_not_propagated: HashSet<String>,
    owner_reference: String,
    #[serde(rename = "categoryTags")]
    category_tags: Vec<String>,
}

#[derive(Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ResumePointResponse {
    pub message_id: Option<String>,
    pub sns_status_code: Option<i16>,
    pub sns_status_text: Option<String>,
    pub aws_request_id: Option<String>,
}

pub struct XvpPlayback {}

impl XvpPlayback {
    fn get_progress_unit(me: &MediaEvent) -> ProgressUnit {
        match (me.completed, me.progress) {
            (true, progress) if progress < 1.0 => ProgressUnit::Percent,
            (true, progress) if progress > 1.0 => ProgressUnit::Seconds,
            // progress == 1.0
            (true, _) => ProgressUnit::Percent,
            (false, progress) if progress < 1.0 => ProgressUnit::Percent,
            (false, progress) if progress > 1.0 => ProgressUnit::Seconds,
            //progress == 1.0
            (false, _) => ProgressUnit::Seconds,
        }
    }

    pub async fn put_resume_point(
        base_url: String,
        params: MediaEventsAccountLinkRequestParams,
    ) -> Result<ResumePointResponse, HttpError> {
        let mut url = Url::parse(&base_url)?;

        url.path_segments_mut()
            .map_err(|_| ParseError::SetHostOnCannotBeABaseUrl)?
            .push("partners")
            .push(&params.dist_session.id)
            .push("accounts")
            .push(&params.dist_session.account_id)
            .push("devices")
            .push(&params.dist_session.device_id)
            .push("resumePoints")
            .push("ott")
            .push(&params.content_partner_id)
            .push(&params.media_event.content_id);

        url.query_pairs_mut().append_pair("clientId", "ripple");

        debug!("Watched data tagged as {:?}", params.data_tags);

        let progress_units = match params.media_event.progress_unit {
            Some(pu) => pu,
            None => XvpPlayback::get_progress_unit(&params.media_event),
        };
        let progress = if matches!(progress_units, ProgressUnit::Percent) {
            params.media_event.progress * 100.0
        } else {
            params.media_event.progress
        };
        let body = ResumePointBody {
            durable_app_id: params.media_event.app_id,
            progress,
            progress_units,
            completed: params.media_event.completed,
            cet: params
                .data_tags
                .iter()
                .map(|tag| tag.tag_name.clone())
                .collect(),
            cet_not_propagated: params
                .data_tags
                .iter()
                .filter(|tag| !tag.propagation_state)
                .map(|tag| tag.tag_name.clone())
                .collect(),
            owner_reference: format!("xrn:subscriber:device:{}", params.dist_session.device_id),
            category_tags: params.category_tags,
        };
        let request_body = serde_json::to_string(&body)?;

        let mut http = HttpClient::new();
        let response_body = http
            .set_token(params.dist_session.token.clone())
            .put(url.into(), request_body)
            .await?;
        let result: Result<ResumePointResponse, serde_json::Error> =
            serde_json::from_str(&response_body);
        result.map_err(|e| {
            error!("ResumePointResponse parse error {:?}", e);
            HttpError::ServiceError
        })
    }
}
