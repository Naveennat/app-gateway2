use crate::{
    model::auth::GetOttTokenResponse,
    rpc::eos_authentication_rpc::{TokenResult, TokenType},
};
use chrono::{DateTime, Utc};

use std::time::{Duration, SystemTime};

use serde::Deserialize;
use serde_json::Value;
use thunder_ripple_sdk::ripple_sdk::log::error;

use crate::gateway::appsanity_gateway::{defaults, AppsanityConfig};

#[derive(Debug)]
pub enum ConvertError {
    IncompatibleFormat,
}

pub fn convert_into_platform_token_response(token: String) -> TokenResult {
    TokenResult {
        value: token,
        expires: None,
        _type: TokenType::Platform,
        scope: None,
        expires_in: None,
        token_type: None,
    }
}

pub fn convert_into_ott_token_response(ott_token: GetOttTokenResponse) -> TokenResult {
    let t = TokenResult {
        value: ott_token.access_token,
        expires: (|| {
            let expiry_system_time = SystemTime::now()
                + Duration::from_secs(ott_token.expires_in.try_into().unwrap_or(0));
            let expiry_date_time: DateTime<Utc> = DateTime::from(expiry_system_time);
            Some(expiry_date_time.to_rfc3339())
        })(),
        _type: TokenType::Distributor,
        scope: Some(ott_token.scope),
        expires_in: Some(ott_token.expires_in),
        token_type: Some(ott_token.token_type),
    };
    t
}

#[derive(Deserialize, Debug, Clone)]
pub struct DistributorConfig {
    pub appsanity: AppsanityConfig,
}

impl DistributorConfig {
    pub fn get(maybe_value: Option<Value>) -> AppsanityConfig {
        if let Some(v) = maybe_value {
            match serde_json::from_value::<DistributorConfig>(v) {
                Ok(dc) => return dc.appsanity,
                Err(e) => error!("Failed to parse config {}", e.to_string()),
            }
        }

        defaults()
    }
}
