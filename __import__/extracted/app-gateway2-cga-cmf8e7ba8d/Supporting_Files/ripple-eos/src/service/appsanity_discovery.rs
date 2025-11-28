use crate::client::xvp_session::XvpSession;
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_discovery::{
    ClearContentSetParams, ContentAccessListSetParams, ContentAccessResponse,
};
use thunder_ripple_sdk::ripple_sdk::utils::error::RippleError;

#[derive(Clone)]
pub struct DiscoveryService {
    pub endpoint: String,
}

impl DiscoveryService {
    pub fn new(endpoint: String) -> DiscoveryService {
        DiscoveryService { endpoint }
    }

    pub async fn set_content_access(
        &self,
        params: ContentAccessListSetParams,
    ) -> Result<ContentAccessResponse, RippleError> {
        match XvpSession::set_content_access(self.endpoint.clone(), params).await {
            Ok(_) => Ok(ContentAccessResponse {}),
            Err(_) => Err(RippleError::ServiceError),
        }
    }

    pub async fn clear_content_access(
        &self,
        params: ClearContentSetParams,
    ) -> Result<(), RippleError> {
        match XvpSession::clear_content_access(self.endpoint.clone(), params).await {
            Ok(_) => Ok(()),
            Err(_) => Err(RippleError::ServiceError),
        }
    }
}
