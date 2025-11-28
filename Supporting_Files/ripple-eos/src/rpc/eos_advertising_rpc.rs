// Copyright 2023 Comcast Cable Communications Management, LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
//
use crate::service::appsanity_advertising::AppsanityAdvertisingService;
use crate::state::distributor_state::DistributorState;
use crate::{
    service::appsanity_advertising::XvpServiceUrl, util::service_util::create_grpc_client_session,
};
use base64::{engine::general_purpose::STANDARD as base64, Engine};
use jsonrpsee::{
    core::{async_trait, Error, RpcResult},
    proc_macros::rpc,
};
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::HashMap;
use thunder_ripple_sdk::ripple_sdk::{
    api::{
        config::Config,
        firebolt::{
            fb_advertising::{
                AdConfig, AdConfigRequestParams, AdConfigResponse, AdIdRequestParams,
                AdvertisingFrameworkConfig, GetAdConfig,
            },
            fb_capabilities::{CapabilityRole, FireboltCap, RoleInfo},
        },
        gateway::rpc_gateway_api::{CallContext, RpcRequest},
    },
    extn::extn_client_message::ExtnResponse,
    log::error,
};

const ADVERTISING_APP_BUNDLE_ID_SUFFIX: &str = "Comcast";
const IFA_ZERO_BASE64: &str = "eyJ4aWZhIjoiMDAwMDAwMDAtMDAwMC0wMDAwLTAwMDAtMDAwMDAwMDAwMDAwIiwieGlmYVR5cGUiOiJzZXNzaW9uSWQiLCJsbXQiOiIwIn0K";

#[derive(Debug)]
pub struct AdvertisingId {
    pub ifa: String,
    pub ifa_type: String,
    pub lmt: String,
}

impl Serialize for AdvertisingId {
    fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        let mut map = serde_json::Map::new();
        map.insert("ifa".to_string(), self.ifa.clone().into());
        // include both ifaType and ifa_type for backward compatibility
        map.insert("ifaType".to_string(), self.ifa_type.clone().into());
        map.insert("ifa_type".to_string(), self.ifa_type.clone().into());
        map.insert("lmt".to_string(), self.lmt.clone().into());
        map.serialize(serializer)
    }
}

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
pub struct AdvertisingPolicy {
    pub skip_restriction: String,
    pub limit_ad_tracking: bool,
}

#[derive(Debug, Deserialize, Clone, Default)]
pub struct AdvertisingIdRPCRequest {
    pub options: Option<ScopeOption>,
}

#[derive(Debug, Deserialize, Clone, Serialize)]
pub struct ScopeOption {
    pub scope: Option<Scope>,
}

#[derive(Debug, Deserialize, Clone, Serialize)]
pub struct Scope {
    #[serde(rename = "type")]
    pub _type: ScopeType,
    pub id: String,
}

#[derive(Debug, Deserialize, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub enum ScopeType {
    Browse,
    Content,
}

impl ScopeType {
    pub fn as_string(&self) -> &'static str {
        match self {
            ScopeType::Browse => "browse",
            ScopeType::Content => "content",
        }
    }
}

fn get_scope_option_map(options: &Option<ScopeOption>) -> HashMap<String, String> {
    let mut scope_option_map = HashMap::new();
    if let Some(scope_opt) = options {
        if let Some(scope) = &scope_opt.scope {
            scope_option_map.insert("type".to_string(), scope._type.as_string().to_string());
            scope_option_map.insert("id".to_string(), scope.id.to_string());
        }
    }
    scope_option_map
}

#[derive(Clone)]
pub struct AdvertisingImpl {
    pub state: DistributorState,
    pub service: AppsanityAdvertisingService,
}

impl AdvertisingImpl {
    pub(crate) fn new(state: &DistributorState) -> Self {
        let config = state.config.clone();
        let service = AppsanityAdvertisingService::new(
            create_grpc_client_session(config.ad_platform_service.url.clone()),
            XvpServiceUrl {
                xvp_xifa_url: config.xvp_xifa_service.url.clone(),
            },
        );
        AdvertisingImpl {
            state: state.clone(),
            service,
        }
    }
}

#[rpc(server)]
pub trait Advertising {
    #[method(name = "advertising.resetIdentifier")]
    async fn reset_identifier(&self, ctx: CallContext) -> RpcResult<()>;
    #[method(name = "advertising.advertisingId")]
    async fn advertising_id(
        &self,
        ctx: CallContext,
        request: Option<AdvertisingIdRPCRequest>,
    ) -> RpcResult<AdvertisingId>;
    #[method(name = "advertising.config")]
    async fn config(
        &self,
        ctx: CallContext,
        config: GetAdConfig,
    ) -> RpcResult<AdvertisingFrameworkConfig>;
    #[method(name = "advertising.deviceAttributes")]
    async fn device_attributes(&self, ctx: CallContext) -> RpcResult<Value>;
    #[method(name = "advertising.appBundleId")]
    fn app_bundle_id(&self, ctx: CallContext) -> RpcResult<String>;
    #[method(name = "eos.initObject")]
    async fn badger_init_object(
        &self,
        ctx: CallContext,
        config: AdConfig,
    ) -> RpcResult<AdvertisingFrameworkConfig>;
    #[method(name = "eos.deviceAdAttributes")]
    async fn device_ad_attributes(&self, ctx: CallContext) -> RpcResult<String>;
}

#[async_trait]
impl AdvertisingServer for AdvertisingImpl {
    async fn reset_identifier(&self, _ctx: CallContext) -> RpcResult<()> {
        match self.state.get_account_session() {
            Some(session) => {
                if let Ok(_) = self.service.clone().reset_ad_identifier(session).await {
                    Ok(())
                } else {
                    Err(jsonrpsee::core::Error::Custom("reset_ad_id failed".into()))
                }
            }
            None => Err(jsonrpsee::core::Error::Custom(
                "Error: failed to get account session".into(),
            )),
        }
    }

    async fn advertising_id(
        &self,
        ctx: CallContext,
        request: Option<AdvertisingIdRPCRequest>,
    ) -> RpcResult<AdvertisingId> {
        let mut client = self.state.get_client().clone();
        let opts = match request {
            Some(r) => r.options,
            None => None,
        };

        let opts_clone = opts.clone();

        let privacydata_req = RpcRequest::get_new_internal_with_appid(
            "ripple.getAllowAppContentAdTargettingSettings".into(),
            opts.map(|scp_opt| serde_json::to_value(scp_opt).unwrap_or_default()),
            &ctx,
        );

        match client
            .request_with_timeout_main(privacydata_req, 5000, Some(ctx.request_id.clone()))
            .await
        {
            Ok(ExtnResponse::Value(r)) => match r {
                Value::Object(privacy_data) => {
                    let privacy_data_map: HashMap<String, String> = privacy_data
                        .into_iter()
                        .filter_map(|(k, v)| v.as_str().map(|v_str| (k, v_str.to_string())))
                        .collect();

                    let session = match self.state.get_account_session() {
                        Some(session) => session,
                        None => {
                            return Err(jsonrpsee::core::Error::Custom(
                                "AccountSession Not available".into(),
                            ));
                        }
                    };

                    let result = self
                        .service
                        .clone()
                        .get_ad_identifier(AdIdRequestParams {
                            privacy_data: privacy_data_map,
                            app_id: ctx.app_id.to_owned(),
                            dist_session: session,
                            scope: get_scope_option_map(&opts_clone),
                        })
                        .await;

                    match result {
                        Ok(res) => Ok(AdvertisingId {
                            ifa: res.ifa,
                            ifa_type: res.ifa_type,
                            lmt: res.lmt,
                        }),
                        Err(_) => Err(jsonrpsee::core::Error::Custom(
                            "Failed to get AdId response data".into(),
                        )),
                    }
                }
                //if Value comes other than String consider as error
                _ => Err(jsonrpsee::core::Error::Custom(
                    "Failed to get privacy data".into(),
                )),
            },
            _ => Err(jsonrpsee::core::Error::Custom(
                "Failed to get ExtnResponse for privacy data".into(),
            )),
        }
    }

    async fn config(
        &self,
        ctx: CallContext,
        config: GetAdConfig,
    ) -> RpcResult<AdvertisingFrameworkConfig> {
        let mut client = self.state.get_client().clone();
        let session = match self.state.get_account_session() {
            Some(session) => session,
            None => {
                return Err(jsonrpsee::core::Error::Custom(
                    "AccountSession Not available".into(),
                ));
            }
        };
        let durable_app_id = ctx.app_id.to_string();
        let environment = config.options.environment;
        let role = RoleInfo {
            role: Some(CapabilityRole::Use),
            capability: FireboltCap::short("advertising:identifier".to_string()),
        };
        let distributor_app_id = match client
            .request_with_timeout_main(
                Config::DistributorExperienceId,
                5000,
                Some(ctx.request_id.clone()),
            )
            .await
        {
            Ok(ExtnResponse::String(distributor_app_id)) => distributor_app_id,
            _ => {
                return Err(jsonrpsee::core::Error::Custom(
                    "Failed to retrieve valid distributor_app_id".into(),
                ));
            }
        };

        let capabilitypermit_req = RpcRequest::get_new_internal_with_appid(
            "ripple.isPermitted".into(),
            match serde_json::to_value(role) {
                Ok(value) => Some(value),
                Err(e) => {
                    error!("Failed to serialize role: {}", e);
                    None
                }
            },
            &ctx,
        );

        let is_permitted_flag = match client
            .request_with_timeout_main(capabilitypermit_req, 5000, Some(ctx.request_id.clone()))
            .await
        {
            Ok(ExtnResponse::Value(Value::Bool(permitted))) => permitted,
            _ => {
                return Err(jsonrpsee::core::Error::Custom(
                    "Failed to get capability permit status".into(),
                ));
            }
        };

        let privacydata_req = RpcRequest::get_new_internal_with_appid(
            "ripple.getAllowAppContentAdTargetting".into(),
            None,
            &ctx,
        );

        let ad_opt_out: bool = !match client
            .request_with_timeout_main(privacydata_req, 5000, Some(ctx.request_id.clone()))
            .await
        {
            Ok(ExtnResponse::Value(Value::Bool(opt_out))) => opt_out,
            _ => {
                error!("Failed to get ad opt-out status, defaulting to false");
                false
            }
        };

        let privacysettings_req = RpcRequest::get_new_internal_with_appid(
            "ripple.getAllowAppContentAdTargettingSettings".into(),
            None,
            &ctx,
        );

        let mut privacydata: HashMap<String, String> = match client
            .request_with_timeout_main(privacysettings_req, 5000, Some(ctx.request_id.clone()))
            .await
        {
            Ok(ExtnResponse::Value(Value::Object(privacy_data))) => {
                let parsed_data: HashMap<String, String> = privacy_data
                    .into_iter()
                    .filter_map(|(k, v)| v.as_str().map(|v_str| (k, v_str.to_string())))
                    .collect();
                parsed_data
            }
            _ => {
                return Err(jsonrpsee::core::Error::Custom(
                    "Failed to get privacy data".into(),
                ));
            }
        };
        privacydata.insert("pdt".into(), "gdp:v1".into());
        let coppa = match config.options.coppa {
            Some(c) => c as u32,
            None => 0,
        };

        let resp = self
            .service
            .clone()
            .get_ad_config(AdConfigRequestParams {
                privacy_data: privacydata.clone(),
                durable_app_id: durable_app_id.clone(),
                dist_session: session,
                environment: environment.to_string(),
                scope: get_scope_option_map(&None),
            })
            .await;

        let adconfig_resp = match resp {
            Ok(res) => res,
            Err(_) => {
                error!("Failed to get AdConfigResponse, return default values");
                AdConfigResponse::default()
            }
        };

        let privacy_data_enc =
            base64.encode(serde_json::to_string(&privacydata).unwrap_or_default());

        let ad_framework_config = AdvertisingFrameworkConfig {
            ad_server_url: adconfig_resp.ad_server_url,
            ad_server_url_template: adconfig_resp.ad_server_url_template,
            ad_network_id: adconfig_resp.ad_network_id,
            ad_profile_id: adconfig_resp.ad_profile_id,
            ad_site_section_id: adconfig_resp.ad_site_section_id,
            ad_opt_out,
            privacy_data: privacy_data_enc,
            ifa: if is_permitted_flag {
                adconfig_resp.ifa
            } else {
                IFA_ZERO_BASE64.to_string()
            },
            ifa_value: if is_permitted_flag {
                adconfig_resp.ifa_value
            } else {
                let ifa_val_zero = adconfig_resp
                    .ifa_value
                    .chars()
                    .map(|x| match x {
                        '-' => x,
                        _ => '0',
                    })
                    .collect();
                ifa_val_zero
            },
            app_name: durable_app_id.clone(),
            app_bundle_id: adconfig_resp.app_bundle_id,
            distributor_app_id: distributor_app_id,
            device_ad_attributes: String::default(),
            coppa,
            authentication_entity: config.options.authentication_entity.unwrap_or_default(),
        };

        Ok(ad_framework_config)
    }

    async fn device_attributes(&self, ctx: CallContext) -> RpcResult<Value> {
        let afc = self.config(ctx.clone(), Default::default()).await?;

        let buff = base64.decode(afc.device_ad_attributes).unwrap_or_default();
        match String::from_utf8(buff) {
            Ok(mut b_string) => {
                if b_string.trim().is_empty() {
                    b_string = "{}".to_string();
                };

                match serde_json::from_str(b_string.as_str()) {
                    Ok(js) => Ok(js),
                    Err(_e) => Err(Error::Custom(String::from("Invalid JSON"))),
                }
            }
            Err(_e) => Err(Error::Custom(String::from("Found invalid UTF-8"))),
        }
    }

    fn app_bundle_id(&self, ctx: CallContext) -> RpcResult<String> {
        Ok(format!(
            "{}.{}",
            ctx.app_id, ADVERTISING_APP_BUNDLE_ID_SUFFIX
        ))
    }

    async fn badger_init_object(
        &self,
        ctx: CallContext,
        options: AdConfig,
    ) -> RpcResult<AdvertisingFrameworkConfig> {
        if options.coppa.is_none() {
            return Err(jsonrpsee::core::Error::Custom(
                "Error: COPPA value is required".into(),
            ));
        }

        let res = self.config(ctx, GetAdConfig { options }).await;

        if let Ok(ad_framework_config) = res {
            return Ok(ad_framework_config);
        }
        Err(jsonrpsee::core::Error::Custom(
            "Error: Device Id not available".into(),
        ))
    }

    async fn device_ad_attributes(&self, ctx: CallContext) -> RpcResult<String> {
        let res = self.device_attributes(ctx.clone()).await;
        match res {
            Ok(Value::String(s)) => Ok(s),
            _ => Ok("".into()),
        }
    }
}
