use std::sync::Arc;

use crate::message::DpabError;
use crate::model::auth::{
    AuthService, GetOttTokenParams, GetOttTokenResponse, GetPlatformTokenParams,
};
use async_trait::async_trait;
use log::error;

use super::thor_permission::ThorPermissionService;

use super::ott_token::OttTokenService;

pub struct AppsanityAuthService {
    thor_permission_service: Arc<ThorPermissionService>,
    ott_token_service: Arc<OttTokenService>,
}

/*
impl AppsanityAuthService {
    fn new(
        thor_permission_service: ThorPermissionService,
        ott_token_service: OttTokenService,
    ) -> Box<Self>
    where
        Self: AuthService<'static>,
    {
        Box::new(AppsanityAuthService {
            thor_permission_service: Arc::new(thor_permission_service),
            ott_token_service: Arc::new(ott_token_service),
        })
    }
}
*/
#[async_trait]
impl AuthService<'_> for AppsanityAuthService {
    async fn get_platform_token(
        self: Box<Self>,
        params: GetPlatformTokenParams,
    ) -> Result<String, DpabError> {
        match self
            .thor_permission_service
            .clone()
            .get_thor_token(
                &params.dist_session,
                params.app_id,
                params.content_provider,
                params.device_session_id,
                params.app_session_id,
            )
            .await
        {
            Ok(token) => Ok(token),
            Err(thor_error) => {
                error!(
                    "Thor Permission Service returned an error, err={:?}",
                    thor_error
                );
                Err(DpabError::ServiceError)
            }
        }
    }

    async fn get_ott_token(
        self: Box<Self>,
        params: GetOttTokenParams,
    ) -> Result<GetOttTokenResponse, DpabError> {
        match self
            .ott_token_service
            .clone()
            .get_ott_token(&params.dist_session, params.app_id, params.xact)
            .await
        {
            Ok(response) => Ok(response),
            Err(err) => {
                error!("Server returned an error, err={:?}", err);
                Err(DpabError::ServiceError)
            }
        }
    }
}
