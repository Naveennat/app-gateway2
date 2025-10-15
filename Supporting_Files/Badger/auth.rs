use async_trait::async_trait;
use thunder_ripple_sdk::ripple_sdk::api::session::AccountSession;

use crate::message::DpabError;

use super::{permissions::api_grants::ApiName, permissions::firebolt::FireboltPermission};

#[derive(Debug)]
pub enum AuthRequest {
    GetPlatformToken(GetPlatformTokenParams),
    GetOttToken(GetOttTokenParams),
}

#[derive(Debug)]
pub struct GetAppPermissionsParams {
    pub app_id: String,
    pub dist_session: AccountSession,
}

#[derive(Debug)]
pub struct GetPlatformTokenParams {
    pub app_id: String,
    pub content_provider: String,
    pub device_session_id: String,
    pub app_session_id: String,
    pub dist_session: AccountSession,
}

#[derive(Debug, Default)]
pub struct GetOttTokenParams {
    pub app_id: String,
    pub xact: String,
    pub dist_session: AccountSession,
}
#[derive(Debug)]
pub struct GetAppMethodPermissionParams {
    pub app_id: String,
    pub distributor_session: AccountSession,
    pub method: ApiName,
}

#[derive(Debug)]
pub struct AppPermissions {
    pub partner_id: String,
    pub permissions: Vec<FireboltPermission>,
}

#[derive(Debug)]
pub enum GetAppMethodPermissionsError {
    ApiAuthenticationFailed,
    ApiAccessGrantFailed,
    ServiceCallFailed { provider: String, context: String },
}

#[async_trait]
pub trait AppPermissionsProvider {
    async fn permissions(&self, app_id: String, partner_id: String) -> AppPermissions;
}

#[async_trait]
pub trait AuthService<'a> {
    async fn get_platform_token(
        self: Box<Self>,
        get_platform_token_params: GetPlatformTokenParams,
    ) -> Result<String, DpabError>;
    async fn get_ott_token(
        self: Box<Self>,
        ott_token_params: GetOttTokenParams,
    ) -> Result<GetOttTokenResponse, DpabError>;
}

#[derive(Debug)]
pub struct GetOttTokenResponse {
    pub access_token: String,
    pub token_type: String,
    pub scope: String,
    pub expires_in: i32,
    pub tid: String,
}
