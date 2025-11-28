use jsonrpsee::{
    core::{async_trait, RpcResult},
    proc_macros::rpc,
};

use thunder_ripple_sdk::{
    client::device_operator::{DeviceCallRequest, DeviceChannelParams, DeviceOperator},
    ripple_sdk::{
        api::{
            firebolt::fb_capabilities::{FireboltCap, CAPABILITY_NOT_SUPPORTED},
            gateway::rpc_gateway_api::CallContext,
            session::AccountSession,
        },
        chrono::{Duration, Utc},
        log::debug,
        utils::{
            rpc_utils::{rpc_error_with_code, rpc_error_with_code_result},
            time_utils,
        },
    },
};

use super::eos_discovery_rpc::get_app_catalog_id;
use crate::{
    extn::extn_utils::{convert_into_ott_token_response, convert_into_platform_token_response},
    state::distributor_state::DistributorState,
};
use serde::{Deserialize, Serialize};

#[derive(Clone, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct TokenResult {
    pub value: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub expires: Option<String>,
    #[serde(rename = "type")]
    pub _type: TokenType,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub scope: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub expires_in: Option<i32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub token_type: Option<String>,
}

impl std::fmt::Debug for TokenResult {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("TokenResult")
            .field("expires", &self.expires)
            .field("_type", &self._type)
            .field("expires_in", &self.expires_in)
            .field("scope", &self.scope)
            .field("token_type", &self.token_type)
            .finish_non_exhaustive()
    }
}

#[derive(Deserialize, Serialize, Debug)]
pub struct TokenRequest {
    #[serde(rename = "type")]
    pub _type: TokenType,
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct PlatformTokenContext {
    pub app_id: String,
    pub content_provider: String,
    pub device_session_id: String,
    pub app_session_id: String,
    pub dist_session: AccountSession,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
struct GetAppTokenIdentityContext {
    #[serde(rename = "type")]
    _type: String,
    value: String,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
pub(crate) struct GetAppTokenThunderResponse {
    pub(crate) success: bool,
    #[serde(rename = "Xact")]
    pub(crate) token: Option<String>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
struct GetAppTokenThunderRequest {
    pub partner_id: String,
    pub identity_context: GetAppTokenIdentityContext,
}

#[derive(Debug, Serialize, Deserialize)]
pub enum AuthenticationError {
    TokenAquistionError { message: String },
    TokenProccessingError { message: String },
}

impl GetAppTokenThunderRequest {
    fn get(atr: TokenContext) -> GetAppTokenThunderRequest {
        GetAppTokenThunderRequest {
            partner_id: atr.distributor_id,
            identity_context: GetAppTokenIdentityContext {
                _type: "applicationId".into(),
                value: atr.app_id,
            },
        }
    }
}

/**
 * https://developer.comcast.com/firebolt/core/sdk/latest/api/authentication
 */
/*TokenType and Token are Firebolt spec types */
#[derive(Debug, PartialEq, Serialize, Deserialize, Clone, Copy)]
pub enum TokenType {
    #[serde(rename = "platform")]
    Platform,
    #[serde(rename = "device")]
    Device,
    #[serde(rename = "distributor")]
    Distributor,
    #[serde(rename = "root")]
    Root,
}

impl std::fmt::Display for TokenType {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            TokenType::Platform => write!(f, "platform"),
            TokenType::Device => write!(f, "device"),
            TokenType::Distributor => write!(f, "distributor"),
            TokenType::Root => write!(f, "root"),
        }
    }
}

#[derive(Debug, PartialEq, Clone, Serialize, Deserialize)]
pub struct TokenContext {
    pub distributor_id: String,
    pub app_id: String,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct DistributorTokenResponse {
    pub access_token: String,
    pub token_type: String,
    pub scope: String,
    pub expires_in: i32,
    pub tid: String,
}

#[rpc(server)]
pub trait Authentication {
    #[method(name = "authentication.token", param_kind = map)]
    async fn token(&self, ctx: CallContext, x: TokenRequest) -> RpcResult<TokenResult>;

    #[method(name = "authentication.device", param_kind = map)]
    async fn device(&self, ctx: CallContext) -> RpcResult<String>;

    #[method(name = "authentication.session")]
    async fn session(&self, ctx: CallContext) -> RpcResult<String>;

    #[method(name = "eos.OAuthBearerToken")]
    async fn auth_bearer_token(&self, ctx: CallContext) -> RpcResult<DistributorTokenResponse>;

    #[method(name = "eos.refreshPlatformAuthToken", param_kind = map)]
    async fn badger_refresh_platform_auth_token(&self, ctx: CallContext) -> RpcResult<String>;

    #[method(name = "eos.getXact", param_kind = map)]
    async fn badger_get_root_token(&self, ctx: CallContext) -> RpcResult<String>;
}

pub struct AuthenticationImpl {
    pub state: DistributorState,
}

impl AuthenticationImpl {
    pub fn new(state: &DistributorState) -> Self {
        Self {
            state: state.clone(),
        }
    }
}

#[async_trait]
impl AuthenticationServer for AuthenticationImpl {
    async fn auth_bearer_token(&self, ctx: CallContext) -> RpcResult<DistributorTokenResponse> {
        let cap = FireboltCap::Short("token:session".into());
        let manifest = self.state.device_manifest.clone();
        let supported_perms = manifest.get_supported_caps();

        let supported_caps: Vec<FireboltCap> = supported_perms.into_iter().map(|x| x.cap).collect();
        if !supported_caps.contains(&cap) {
            return rpc_error_with_code_result(
                format!("{} is not supported", cap.as_str()),
                CAPABILITY_NOT_SUPPORTED,
            );
        }

        if !supports_distributor_session(&self.state) {
            return Err(Self::send_dist_token_not_supported());
        }

        let cp_id = get_app_catalog_id(self.state.clone(), &ctx).await;

        let dist_session = match self.state.get_account_session() {
            Some(session) => session,
            None => {
                return Err(jsonrpsee::core::Error::Custom(String::from(
                    "Account session is not available",
                )));
            }
        };

        // get root token & get the cima token from ots
        let root_token = self.get_root_token().await?;
        debug!("Getting distributor token for {}", ctx.app_id);
        match self
            .state
            .get_auth_service()
            .get_ott_token_service()
            .get_ott_token(&dist_session, cp_id, root_token.value)
            .await
        {
            Ok(response) => Ok(DistributorTokenResponse {
                access_token: response.access_token,
                token_type: response.token_type,
                scope: response.scope,
                expires_in: response.expires_in,
                tid: response.tid,
            }),
            Err(e) => Err(jsonrpsee::core::Error::Custom(format!(
                "Ripple Error refreshing platform token: {:?}",
                e
            ))),
        }
    }

    async fn badger_refresh_platform_auth_token(&self, ctx: CallContext) -> RpcResult<String> {
        let cap = FireboltCap::Short("token:platform".into());
        let manifest = self.state.device_manifest.clone();
        let supported_perms = manifest.get_supported_caps();

        let supported_caps: Vec<FireboltCap> = supported_perms.into_iter().map(|x| x.cap).collect();
        if !supported_caps.contains(&cap) {
            return rpc_error_with_code_result(
                format!("{} is not supported", cap.as_str()),
                CAPABILITY_NOT_SUPPORTED,
            );
        }

        let cp_id = get_app_catalog_id(self.state.clone(), &ctx).await;

        let dist_session = match self.state.get_account_session() {
            Some(session) => session,
            None => {
                return Err(jsonrpsee::core::Error::Custom(String::from(
                    "Account session is not available",
                )));
            }
        };

        // Acquire platform token using Thor Permission Service (original permission service flow)
        let context = PlatformTokenContext {
            app_id: ctx.app_id.clone(),
            content_provider: cp_id.clone(),
            device_session_id: self.state.metrics.device_session_id.clone().into(),
            app_session_id: ctx.session_id.clone(),
            dist_session: dist_session.clone(),
        };

        debug!("Getting platform token for {} via Permission Service", context.app_id);
        match self
            .state
            .get_auth_service()
            .get_thor_permission_service()
            .get_thor_token(
                &context.dist_session,
                context.app_id,
                context.content_provider,
                context.device_session_id,
                context.app_session_id,
            )
            .await
        {
            Ok(token) => Ok(token),
            Err(e) => Err(jsonrpsee::core::Error::Custom(format!(
                "Ripple Error getting platform token via Permission Service: {:?}",
                e
            ))),
        }
    }

    async fn badger_get_root_token(&self, _ctx: CallContext) -> RpcResult<String> {
        match self.get_root_token().await {
            Ok(token) => Ok(token.value),
            Err(e) => Err(jsonrpsee::core::Error::Custom(format!(
                "Ripple Error refreshing root token: {:?}",
                e
            ))),
        }
    }

    async fn token(&self, ctx: CallContext, token_request: TokenRequest) -> RpcResult<TokenResult> {
        match token_request._type {
            TokenType::Platform => {
                let cap = FireboltCap::Short("token:platform".into());
                let manifest = &self.state.device_manifest;
                let supported_perms = manifest.get_supported_caps();

                let supported_caps: Vec<FireboltCap> =
                    supported_perms.into_iter().map(|x| x.cap).collect();
                if supported_caps.contains(&cap) {
                    self.token(TokenType::Platform, ctx).await
                } else {
                    return rpc_error_with_code_result(
                        format!("{} is not supported", cap.as_str()),
                        CAPABILITY_NOT_SUPPORTED,
                    );
                }
            }
            TokenType::Root => self.get_root_token().await,
            TokenType::Device => self.token(TokenType::Device, ctx).await,
            TokenType::Distributor => {
                let cap = FireboltCap::Short("token:session".into());
                let manifest = &self.state.device_manifest;
                let supported_perms = manifest.get_supported_caps();

                let supported_caps: Vec<FireboltCap> =
                    supported_perms.into_iter().map(|x| x.cap).collect();
                if supported_caps.contains(&cap) {
                    self.token(TokenType::Distributor, ctx).await
                } else {
                    return rpc_error_with_code_result(
                        format!("{} is not supported", cap.as_str()),
                        CAPABILITY_NOT_SUPPORTED,
                    );
                }
            }
        }
    }

    async fn device(&self, ctx: CallContext) -> RpcResult<String> {
        if !supports_device_tokens(&self.state) {
            return Err(Self::send_dist_token_not_supported());
        }
        match self.token(TokenType::Device, ctx).await {
            Ok(r) => Ok(r.value),
            Err(e) => Err(e),
        }
    }

    async fn session(&self, ctx: CallContext) -> RpcResult<String> {
        match self.token(TokenType::Distributor, ctx).await {
            Ok(r) => Ok(r.value),
            Err(e) => Err(e),
        }
    }
}

impl AuthenticationImpl {
    fn send_dist_token_not_supported() -> jsonrpsee::core::Error {
        rpc_error_with_code::<String>(
            "capability xrn:firebolt:capability:token:session is not supported".to_owned(),
            CAPABILITY_NOT_SUPPORTED,
        )
    }

    async fn token(&self, token_type: TokenType, ctx: CallContext) -> RpcResult<TokenResult> {
        if let TokenType::Distributor = &token_type {
            if !supports_distributor_session(&self.state) {
                return Err(Self::send_dist_token_not_supported());
            }
        }

        if let TokenType::Device = &token_type {
            if !supports_device_tokens(&self.state) {
                return Err(Self::send_dist_token_not_supported());
            }
        }

        let cp_id = get_app_catalog_id(self.state.clone(), &ctx).await;

        let dist_session = match self.state.get_account_session() {
            Some(session) => session,
            None => {
                return Err(jsonrpsee::core::Error::Custom(String::from(
                    "Account session is not available",
                )));
            }
        };

        match token_type {
            TokenType::Platform => {
                // Acquire platform token via Thor Permission Service (original permission service flow)
                let context = PlatformTokenContext {
                    app_id: ctx.app_id.clone(),
                    content_provider: cp_id.clone(),
                    device_session_id: self.state.metrics.device_session_id.clone().into(),
                    app_session_id: ctx.session_id.clone(),
                    dist_session,
                };
                debug!("Getting platform token for {}", context.app_id);
                self.state
                    .get_auth_service()
                    .get_thor_permission_service()
                    .get_thor_token(
                        &context.dist_session,
                        context.app_id.clone(),
                        context.content_provider.clone(),
                        context.device_session_id.clone(),
                        context.app_session_id.clone(),
                    )
                    .await
                    .map(|token| convert_into_platform_token_response(token))
                    .map_err(|e| jsonrpsee::core::Error::Custom(format!("{:?}", e)))
            }
            TokenType::Distributor => {
                // get root token & get the cima token from ots
                let root_token = self.get_root_token().await?;
                self.state
                    .get_auth_service()
                    .get_ott_token_service()
                    .get_ott_token(&dist_session, cp_id, root_token.value)
                    .await
                    .map(|response| convert_into_ott_token_response(response))
                    .map_err(|e| jsonrpsee::core::Error::Custom(format!("{:?}", e)))
            }
            _ => {
                // get device token from thunder
                let context = TokenContext {
                    distributor_id: dist_session.id,
                    app_id: ctx.app_id,
                };
                get_device_token(&self.state, Some(context)).await
            }
        }
    }

    async fn get_root_token(&self) -> RpcResult<TokenResult> {
        if let Some(token) = self.state.get_root_device_token() {
            return Ok(token);
        }

        match get_token(&self.state, true).await {
            Ok(token_result) => {
                self.state.update_root_device_token(token_result.clone());
                Ok(token_result)
            }
            Err(e) => Err(jsonrpsee::core::Error::Custom(format!(
                "Error retrieving {:?} token: {:?}",
                TokenType::Root,
                e
            ))),
        }
    }
}

async fn get_device_token(
    state: &DistributorState,
    context: Option<TokenContext>,
) -> RpcResult<TokenResult> {
    let client = state.get_thunder_client();
    if let Some(t) = context {
        let get_app_token_request = GetAppTokenThunderRequest::get(t);
        let device_request = DeviceCallRequest {
            method: "org.rdk.SecManager.1.getDeviceIdentity".into(),
            params: Some(DeviceChannelParams::Json(
                serde_json::to_string(&get_app_token_request).unwrap(),
            )),
        };
        let resp = client.call(device_request).await;
        debug!("{}", resp.message);

        match serde_json::from_value::<GetAppTokenThunderResponse>(resp.message) {
            Ok(result) => {
                if result.success {
                    if let Some(token) = result.token {
                        Ok(TokenResult {
                            value: token,
                            expires: None,
                            _type: TokenType::Device,
                            scope: None,
                            expires_in: None,
                            token_type: None,
                        })
                    } else {
                        Err(jsonrpsee::core::Error::Custom(String::from(
                            "Token is missing in the result",
                        )))
                    }
                } else {
                    Err(jsonrpsee::core::Error::Custom(format!(
                        "Error retrieving device token: {:?}",
                        result
                    )))
                }
            }
            Err(e) => Err(jsonrpsee::core::Error::Custom(format!(
                "Error retrieving device token: {:?}",
                e
            ))),
        }
    } else {
        Err(jsonrpsee::core::Error::Custom(String::from(
            "TokenContext is missing",
        )))
    }
}

pub(crate) async fn get_token(
    state: &DistributorState,
    get_expires: bool,
) -> Result<TokenResult, AuthenticationError> {
    let request = DeviceCallRequest {
        method: "org.rdk.AuthService.getAuthToken".into(),
        params: None,
    };
    let response = state.get_thunder_client().call(request).await;

    let t = match response.message["token"].as_str() {
        Some(tok) => String::from(tok),
        None => {
            return Err(AuthenticationError::TokenAquistionError {
                message: "empty token returned from Thunder".to_string(),
            })
        }
    };

    if get_expires {
        match response.message["expires"].as_i64() {
            Some(exp) => {
                return Ok(TokenResult {
                    value: t,
                    expires: Some(time_utils::timestamp_2_iso8601(expiration_date(exp))),
                    _type: TokenType::Root,
                    scope: None,
                    expires_in: None,
                    token_type: None,
                })
            }
            None => {
                return Err(AuthenticationError::TokenAquistionError {
                    message: "empty expiration date returned from Thunder".to_string(),
                })
            }
        }
    }
    Ok(TokenResult {
        value: t,
        expires: None,
        _type: TokenType::Root,
        scope: None,
        expires_in: None,
        token_type: None,
    })
}

fn supports_device_tokens(state: &DistributorState) -> bool {
    state.get_client().get_bool_config("supports_device_tokens")
}

fn supports_distributor_session(state: &DistributorState) -> bool {
    state
        .get_client()
        .get_bool_config("supports_distributor_session")
}

fn expiration_date(offset: i64) -> i64 {
    /* offset is string containing millisecond offset from now to time of expiration*/
    let offset_seconds = offset / 1000;
    (Utc::now() + Duration::seconds(offset_seconds)).timestamp()
}
