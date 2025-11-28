use std::collections::HashMap;

use crate::client::xvp_playback::XvpPlayback;
use crate::extn::eos_ffi::EOS_DISTRIBUTOR_SERVICE_ID;

use crate::manager::data_governor::DataGovernance;
use crate::util::service_util::get_age_policy_identifiers;
use crate::{
    client::xvp_videoservice::XvpVideoService, service::appsanity_discovery::DiscoveryService,
    state::distributor_state::DistributorState,
};
use jsonrpsee::{core::RpcResult, proc_macros::rpc};
use log::{debug, error, info};
use ripple_sdk::service::service_message::JsonRpcMessage;
use serde::Deserialize;
use serde::Deserializer;
use serde_json::json;
use thunder_ripple_sdk::ripple_sdk::api::account_link::WatchedRequest;
use thunder_ripple_sdk::ripple_sdk::api::device::entertainment_data::ContentIdentifiers;
use thunder_ripple_sdk::ripple_sdk::api::distributor::distributor_privacy::DataEventType;
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_discovery::ClearContentSetParams;
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_discovery::DataTagInfo;
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_discovery::EntitlementData;
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_discovery::LocalizedString;
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_discovery::MediaEvent;
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_discovery::MediaEventsAccountLinkRequestParams;
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_discovery::ProgressUnit;
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_discovery::SignInInfo;
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_discovery::WatchNextInfo;
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_discovery::WatchedInfo;
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_discovery::{
    ContentAccessAvailability, ContentAccessEntitlement, ContentAccessInfo, EntitlementsInfo,
};
use thunder_ripple_sdk::ripple_sdk::api::{
    apps::AppEvent,
    firebolt::{
        fb_discovery::{
            ContentAccessListSetParams, ContentAccessRequest, SessionParams, SignInRequestParams,
        },
        fb_general::{ListenRequest, ListenRequestWithEvent, ListenerResponse},
    },
    gateway::rpc_gateway_api::{CallContext, RpcRequest},
};
use thunder_ripple_sdk::ripple_sdk::async_trait::async_trait;
use thunder_ripple_sdk::ripple_sdk::utils::rpc_utils::rpc_err;
use thunder_ripple_sdk::ripple_sdk::utils::rpc_utils::rpc_error_with_code;
use thunder_ripple_sdk::ripple_sdk::utils::time_utils::convert_timestamp_to_iso8601;
use thunder_ripple_sdk::ripple_sdk::JsonRpcErrorType;

use super::eos_metrics_rpc::BadgerEmptyResult;

pub const EVENT_ON_SIGN_IN: &str = "discovery.onSignIn";
pub const EVENT_ON_SIGN_OUT: &str = "discovery.onSignOut";

const BADGER_MEDIA_EVENT_PERCENT: &str = "percent";
const BADGER_MEDIA_EVENT_SECONDS: &str = "seconds";
const BADGER_ENTITLEMENT_SIGNIN: &str = "signIn";
const BADGER_ENTITLEMENT_SIGNOUT: &str = "signOut";
const BADGER_ENTITLEMENT_APPLAUNCH: &str = "appLaunch";
const BADGER_ENTITLEMENTS_UPDATE: &str = "entitlementsUpdate";
const BADGER_ENTITLEMENTS_ACCOUNTLINK: &str = "accountLink";
const DOWNSTREAM_SERVICE_UNAVAILABLE_ERROR_CODE: i32 = -50200;

pub struct DiscoveryImpl {
    pub state: DistributorState,
    pub service: DiscoveryService,
}

#[derive(Debug, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct SubscriptionEntitlements {
    pub id: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub start_date: Option<i64>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub end_date: Option<i64>,
}

impl From<SubscriptionEntitlements> for EntitlementData {
    fn from(sub_entitlement: SubscriptionEntitlements) -> Self {
        EntitlementData {
            entitlement_id: sub_entitlement.id.clone(),
            start_time: sub_entitlement.start_date.map(convert_timestamp_to_iso8601),
            end_time: sub_entitlement.end_date.map(convert_timestamp_to_iso8601),
        }
    }
}

enum DeserializeEnum {
    Action,
    Type,
    ProgressUnit,
}

fn deserialize_option<'de, D>(
    deserializer: D,
    enum_value: DeserializeEnum,
) -> Result<Option<String>, D::Error>
where
    D: Deserializer<'de>,
{
    let value = String::deserialize(deserializer)?;
    let valid_entries = create_valid_list(enum_value);
    if valid_entries.contains(&value.as_str()) {
        Ok(Some(value))
    } else {
        Err(serde::de::Error::custom(format!(
            "Invalid value ({})",
            value
        )))
    }
}

fn create_valid_list(enum_value: DeserializeEnum) -> Vec<&'static str> {
    let mut valid_entry = Vec::new();
    match enum_value {
        DeserializeEnum::Action => {
            valid_entry.push(BADGER_ENTITLEMENT_SIGNIN);
            valid_entry.push(BADGER_ENTITLEMENT_SIGNOUT);
            valid_entry.push(BADGER_ENTITLEMENT_APPLAUNCH);
        }
        DeserializeEnum::Type => {
            valid_entry.push(BADGER_ENTITLEMENTS_UPDATE);
            valid_entry.push(BADGER_ENTITLEMENTS_ACCOUNTLINK);
        }
        DeserializeEnum::ProgressUnit => {
            valid_entry.push(BADGER_MEDIA_EVENT_PERCENT);
            valid_entry.push(BADGER_MEDIA_EVENT_SECONDS);
        }
    }
    valid_entry
}

pub fn link_action_deserialize<'de, D>(deserializer: D) -> Result<Option<String>, D::Error>
where
    D: Deserializer<'de>,
{
    deserialize_option(deserializer, DeserializeEnum::Action)
}

pub fn link_type_deserialize<'de, D>(deserializer: D) -> Result<Option<String>, D::Error>
where
    D: Deserializer<'de>,
{
    deserialize_option(deserializer, DeserializeEnum::Type)
}

#[derive(Debug, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct BadgerEntitlementsAccountLinkRequest {
    #[serde(
        default,
        skip_serializing_if = "Option::is_none",
        deserialize_with = "link_action_deserialize"
    )]
    pub action: Option<String>, /*signIn, signOut, appLaunch,  */
    #[serde(
        default,
        rename = "type",
        skip_serializing_if = "Option::is_none",
        deserialize_with = "link_type_deserialize"
    )]
    pub link_type: Option<String>, /* entitlementsUpdate */
    #[serde(skip_serializing_if = "Option::is_none")]
    pub subscription_entitlements: Option<Vec<SubscriptionEntitlements>>,
}

#[derive(Debug, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct BadgerLaunchpadTile {
    pub launchpad_tile: BadgerLaunchpadTileData,
}

#[derive(Debug, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct BadgerLaunchpadTileData {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub titles: Option<LocalizedString>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub url: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub expiration: Option<i64>,
    pub content_id: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub image: Option<HashMap<String, HashMap<String, String>>>,
}

pub fn progress_unit_deserialize<'de, D>(deserializer: D) -> Result<Option<String>, D::Error>
where
    D: Deserializer<'de>,
{
    deserialize_option(deserializer, DeserializeEnum::ProgressUnit)
}

pub fn progress_value_deserialize<'de, D>(deserializer: D) -> Result<f32, D::Error>
where
    D: Deserializer<'de>,
{
    let value: f32 = f32::deserialize(deserializer)?;
    if value < 0.0 {
        Err(serde::de::Error::custom(
            "Invalid value for progress. Minimum value should be 0.0",
        ))
    } else {
        Ok(value)
    }
}

#[derive(Debug, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct BadgerMediaEventData {
    pub content_id: String,
    pub completed: bool,
    #[serde(default, deserialize_with = "progress_value_deserialize")]
    pub progress: f32,
    #[serde(
        default,
        skip_serializing_if = "Option::is_none",
        deserialize_with = "progress_unit_deserialize"
    )]
    pub progress_units: Option<String>,
}

#[derive(Debug, Deserialize, Clone)]
pub struct BadgerMediaEvent {
    pub event: BadgerMediaEventData,
}

#[rpc(server)]
pub trait Discovery {
    #[method(name = "discovery.signIn")]
    async fn sign_in(&self, ctx: CallContext, sign_in_info: SignInInfo) -> RpcResult<bool>;

    #[method(name = "discovery.signOut")]
    async fn sign_out(&self, ctx: CallContext) -> RpcResult<bool>;

    #[method(name = "discovery.onSignIn", aliases = ["discovery.onSignOut"])]
    fn on_sign_in_out(
        &self,
        ctx: CallContext,
        request: ListenRequest,
    ) -> RpcResult<ListenerResponse>;

    #[method(name = "discovery.entitlements")]
    async fn entitlements(
        &self,
        ctx: CallContext,
        entitlements_info: EntitlementsInfo,
    ) -> RpcResult<bool>;

    #[method(name = "discovery.contentAccess")]
    async fn discovery_content_access(
        &self,
        ctx: CallContext,
        request: ContentAccessRequest,
    ) -> RpcResult<()>;

    #[method(name = "discovery.clearContentAccess")]
    async fn discovery_clear_content_access(&self, ctx: CallContext) -> RpcResult<()>;

    #[method(name = "discovery.watched")]
    async fn watched(&self, ctx: CallContext, watched_info: WatchedInfo) -> RpcResult<bool>;

    #[method(name = "discovery.watchNext")]
    async fn watch_next(&self, ctx: CallContext, watch_next_info: WatchNextInfo)
        -> RpcResult<bool>;

    #[method(name = "eos.entitlementsAccountLink")]
    async fn badger_entitlements_account_link(
        &self,
        ctx: CallContext,
        request: BadgerEntitlementsAccountLinkRequest,
    ) -> RpcResult<BadgerEmptyResult>;

    #[method(name = "eos.launchpadAccountLink")]
    async fn badger_launch_pad_account_link(
        &self,
        ctx: CallContext,
        launch_pad_tile_data: BadgerLaunchpadTile,
    ) -> RpcResult<BadgerEmptyResult>;

    #[method(name = "eos.mediaEventAccountLink")]
    async fn badger_media_event_account_link(
        &self,
        ctx: CallContext,
        badger_media_event: BadgerMediaEvent,
    ) -> RpcResult<BadgerEmptyResult>;
}

#[async_trait]
impl DiscoveryServer for DiscoveryImpl {
    async fn sign_in(&self, ctx: CallContext, sign_in_info: SignInInfo) -> RpcResult<bool> {
        info!("Discovery.signIn");
        if sign_in_info.entitlements.is_some() {
            if !self.handle_content_access(&ctx, sign_in_info.into()).await {
                return Err(rpc_downstream_service_err("Received error from Server"));
            }
        }
        Ok(self.handle_sign_in(&ctx, true).await)
    }

    async fn sign_out(&self, ctx: CallContext) -> RpcResult<bool> {
        info!("Discovery.signOut");
        Ok(self.handle_sign_in(&ctx, false).await)
    }

    fn on_sign_in_out(
        &self,
        ctx: CallContext,
        request: ListenRequest,
    ) -> RpcResult<ListenerResponse> {
        Ok(self.handle_listener(&ctx, &request))
    }

    async fn entitlements(
        &self,
        ctx: CallContext,
        entitlements_info: EntitlementsInfo,
    ) -> RpcResult<bool> {
        Ok(self
            .handle_content_access(&ctx, entitlements_info.into())
            .await)
    }

    async fn discovery_content_access(
        &self,
        ctx: CallContext,
        request: ContentAccessRequest,
    ) -> RpcResult<()> {
        if self.handle_content_access(&ctx, request).await {
            Ok(())
        } else {
            Err(rpc_err("service error"))
        }
    }

    async fn discovery_clear_content_access(&self, ctx: CallContext) -> RpcResult<()> {
        if self.handle_clear_content_access(&ctx).await {
            Ok(())
        } else {
            Err(rpc_err("service error"))
        }
    }

    async fn watched(&self, context: CallContext, info: WatchedInfo) -> RpcResult<bool> {
        info!("Discovery.watched");
        let request = WatchedRequest {
            context,
            info,
            unit: None,
        };
        Ok(self.handle_watched(request).await)
    }

    async fn watch_next(
        &self,
        ctx: CallContext,
        watch_next_info: WatchNextInfo,
    ) -> RpcResult<bool> {
        info!("Discovery.watchNext");
        let watched_info = WatchedInfo {
            entity_id: watch_next_info.identifiers.entity_id.unwrap_or_default(),
            progress: 1.0,
            completed: Some(false),
            watched_on: None,
            age_policy: None,
        };
        self.watched(ctx, watched_info).await
    }

    async fn badger_launch_pad_account_link(
        &self,
        ctx: CallContext,
        launch_pad_tile_data: BadgerLaunchpadTile,
    ) -> RpcResult<BadgerEmptyResult> {
        let watch_next_info = WatchNextInfo {
            title: launch_pad_tile_data.launchpad_tile.titles.clone(),
            url: launch_pad_tile_data.launchpad_tile.url.clone(),
            identifiers: ContentIdentifiers {
                asset_id: None,
                entity_id: Some(launch_pad_tile_data.launchpad_tile.content_id.clone()),
                season_id: None,
                series_id: None,
                app_content_data: None,
            },
            expires: launch_pad_tile_data
                .launchpad_tile
                .expiration
                .map(convert_timestamp_to_iso8601),
            images: launch_pad_tile_data.launchpad_tile.image.clone(),
        };

        let info = WatchedInfo {
            entity_id: watch_next_info.identifiers.entity_id.unwrap(),
            progress: 1.00,
            completed: Some(false),
            watched_on: None,
            age_policy: None,
        };
        if let Ok(_) = self.watched(ctx, info).await {
            Ok(BadgerEmptyResult::default())
        } else {
            Err(rpc_err(
                "Could not notify the platform that content was partially or completely watched",
            ))
        }
    }

    async fn badger_media_event_account_link(
        &self,
        ctx: CallContext,
        badger_media_event: BadgerMediaEvent,
    ) -> RpcResult<BadgerEmptyResult> {
        // By default treat it as seconds.
        // If explicitly set media as percent then div by 100 to bring the
        // value between 0.0 and 1.0
        let progress = match (badger_media_event.event.progress_units).as_deref() {
            Some(BADGER_MEDIA_EVENT_PERCENT) => {
                if badger_media_event.event.progress > 1.0_f32 {
                    badger_media_event.event.progress / 100.00
                } else {
                    badger_media_event.event.progress
                }
            }
            _ => badger_media_event.event.progress,
        };
        let info = WatchedInfo {
            entity_id: badger_media_event.event.content_id.to_owned(),
            progress,
            completed: Some(badger_media_event.event.completed),
            watched_on: None,
            age_policy: None,
        };
        let progress_unit = match (badger_media_event.event.progress_units).as_deref() {
            Some(BADGER_MEDIA_EVENT_PERCENT) => Some(ProgressUnit::Percent),

            Some(BADGER_MEDIA_EVENT_SECONDS) => Some(ProgressUnit::Seconds),
            _ => None,
        };
        let watched_request = WatchedRequest {
            context: ctx.clone(),
            info,
            unit: progress_unit,
        };

        if self.handle_watched(watched_request).await {
            Ok(BadgerEmptyResult::default())
        } else {
            Err(rpc_err(
                "Could not notify the platform that content was partially or completely watched",
            ))
        }
    }

    async fn badger_entitlements_account_link(
        &self,
        ctx: CallContext,
        request: BadgerEntitlementsAccountLinkRequest,
    ) -> RpcResult<BadgerEmptyResult> {
        let mut sign_in_resp = true;
        let mut resp = Ok(BadgerEmptyResult::default());
        let mut entitlements_updated = false;

        let action = request.action.as_deref().unwrap_or_default();

        match action {
            BADGER_ENTITLEMENT_SIGNIN => {
                // Sign In
                // Entitlement update
                entitlements_updated = true;
                resp = self
                    .handle_badger_entitlement_update(
                        &ctx,
                        request.subscription_entitlements.clone(),
                    )
                    .await;
                sign_in_resp = self.handle_sign_in(&ctx, true).await;
            }

            BADGER_ENTITLEMENT_SIGNOUT => {
                // Badger clearing entitlement before signing out.
                // Sign Out
                sign_in_resp = self.handle_sign_in(&ctx, false).await;
                resp = if self.handle_clear_content_access(&ctx).await {
                    Ok(BadgerEmptyResult::default())
                } else {
                    Err(rpc_downstream_service_err(
                        "Error: Notifying SignIn info to the platform",
                    ))
                };

                if request.link_type.is_some() {
                    resp = self
                        .handle_badger_entitlement_update(
                            &ctx,
                            request.subscription_entitlements.clone(),
                        )
                        .await;
                }
            }

            BADGER_ENTITLEMENT_APPLAUNCH => {
                // Entitlement update
                entitlements_updated = true;
                resp = self
                    .handle_badger_entitlement_update(
                        &ctx,
                        request.subscription_entitlements.clone(),
                    )
                    .await;
            }
            _ => {}
        };

        if request.link_type.is_some() && !entitlements_updated {
            resp = self
                .handle_badger_entitlement_update(&ctx, request.subscription_entitlements.clone())
                .await;
        }

        if sign_in_resp && resp.is_ok() {
            resp = Ok(BadgerEmptyResult::default());
        } else {
            resp = Err(rpc_downstream_service_err("Received error from Server"));
        }
        resp
    }
}

impl DiscoveryImpl {
    pub fn new(state: &DistributorState) -> Self {
        Self {
            state: state.clone(),
            service: DiscoveryService {
                endpoint: state.config.xvp_session_service.url.clone(),
            },
        }
    }

    async fn handle_sign_in(&self, ctx: &CallContext, is_signed_in: bool) -> bool {
        let mut result = false;
        if let Some(dist_session) = self.state.get_account_session() {
            let params: SignInRequestParams = SignInRequestParams {
                session_info: SessionParams {
                    app_id: ctx.app_id.clone(),
                    dist_session,
                },
                is_signed_in,
            };

            let config = self.state.config.clone();

            result = XvpVideoService::sign_in(
                &config.xvp_video_service.url.to_string(),
                config.xvp_data_scopes.get_xvp_sign_in_state_scope(),
                params,
            )
            .await
            .is_ok();
            if result {
                let app_id = ctx.app_id.clone();
                let client = self.state.get_client();
                let app_event = serde_json::to_value(AppEvent {
                    event_name: if is_signed_in {
                        EVENT_ON_SIGN_IN.to_owned()
                    } else {
                        EVENT_ON_SIGN_OUT.to_owned()
                    },
                    result: json!({"appId": app_id}),
                    context: None,
                    app_id: None,
                })
                .unwrap();
                // dispatch event
                let _ = client.request_transient(RpcRequest::get_new_internal(
                    "ripple.sendAppEvent".to_owned(),
                    Some(app_event),
                ));
            }
        }
        result
    }

    fn handle_listener(
        &self,
        ctx: &CallContext,
        listen_request: &ListenRequest,
    ) -> ListenerResponse {
        let param = serde_json::to_value(ListenRequestWithEvent {
            request: listen_request.clone(),
            event: ctx.method.clone(),
            context: ctx.clone(),
        })
        .unwrap();
        // let rpc_request =
        //     RpcRequest::get_new_internal("ripple.registerAppEvent".to_owned(), Some(param));

        // let _ = self.state.get_client().request_transient(rpc_request);
        // ListenerResponse {
        //     listening: listen_request.listen,
        //     event: ctx.method.clone(),
        // }

        if let Some(service_client) = self.state.get_service_client() {
            let _ = service_client.request_transient(
                "ripple.registerAppEvent".to_string(),
                Some(param),
                Some(ctx),
                EOS_DISTRIBUTOR_SERVICE_ID.to_string(),
            );
        } else {
            error!("Service client is unavailable. Failed to register app event.");
        }
        ListenerResponse {
            listening: listen_request.listen,
            event: ctx.method.clone(),
        }
    }

    async fn handle_content_access(
        &self,
        ctx: &CallContext,
        request: ContentAccessRequest,
    ) -> bool {
        if request.ids.availabilities.is_none() && request.ids.entitlements.is_none() {
            return true;
        } else if let Some(dist_session) = self.state.get_account_session() {
            let params = ContentAccessListSetParams {
                session_info: SessionParams {
                    app_id: ctx.app_id.clone(),
                    dist_session,
                },
                content_access_info: ContentAccessInfo {
                    availabilities: request.ids.availabilities.map(|availability_vec| {
                        availability_vec
                            .into_iter()
                            .map(|x| ContentAccessAvailability {
                                _type: x._type.as_string().to_owned(),
                                id: x.id,
                                catalog_id: x.catalog_id,
                                start_time: x.start_time,
                                end_time: x.end_time,
                            })
                            .collect()
                    }),
                    entitlements: request.ids.entitlements.map(|entitlement_vec| {
                        entitlement_vec
                            .into_iter()
                            .map(|x| ContentAccessEntitlement {
                                entitlement_id: x.entitlement_id,
                                start_time: x.start_time,
                                end_time: x.end_time,
                            })
                            .collect()
                    }),
                },
            };
            return self.service.set_content_access(params).await.is_ok();
        }

        false
    }

    async fn handle_clear_content_access(&self, ctx: &CallContext) -> bool {
        if let Some(dist_session) = self.state.get_account_session() {
            let params = ClearContentSetParams {
                session_info: SessionParams {
                    app_id: ctx.app_id.clone(),
                    dist_session,
                },
            };
            return self.service.clear_content_access(params).await.is_ok();
        }
        false
    }

    async fn handle_badger_entitlement_update(
        &self,
        ctx: &CallContext,
        subscription_entitlements: Option<Vec<SubscriptionEntitlements>>,
    ) -> RpcResult<BadgerEmptyResult> {
        let resp = Ok(BadgerEmptyResult::default());

        if subscription_entitlements.is_none() {
            return resp;
        }
        let ent_data = subscription_entitlements
            .unwrap()
            .into_iter()
            .map(|item| item.into())
            .collect();

        if self
            .handle_content_access(
                ctx,
                EntitlementsInfo {
                    entitlements: ent_data,
                }
                .into(),
            )
            .await
        {
            Ok(BadgerEmptyResult::default())
        } else {
            Err(rpc_downstream_service_err(
                "Error: Notifying SignIn info to the platform",
            ))
        }
    }

    pub async fn handle_watched(&self, request: WatchedRequest) -> bool {
        let watched_info = request.info.clone();
        let ctx = request.context;
        let (data_tags, drop_data) = DataGovernance::resolve_tags(
            &self.state,
            ctx.clone().app_id.clone(),
            DataEventType::Watched,
        )
        .await;
        debug!("drop_all={:?} data_tags={:?}", drop_data, data_tags);
        if drop_data {
            return false;
        }

        // Apply age policy logic if available
        let age_policy_config = &self.state.config.age_policy;
        let session_policies =
            get_age_policy_identifiers(self.state.service_client.clone(), ctx.clone()).await;

        let request_age_policy = watched_info.clone().age_policy;

        let additional_cets = if let Some(cets) =
            age_policy_config.get_policy_cets(request_age_policy.clone(), session_policies)
        {
            cets.into_iter()
                .map(|tag| DataTagInfo {
                    tag_name: tag,
                    propagation_state: true,
                })
                .collect::<Vec<_>>()
        } else {
            Vec::new()
        };
        let category_tags = match age_policy_config.get_category_tags(request_age_policy) {
            Some(tags) => tags.tags,
            None => vec![],
        };

        // Combine data_tags with age policy CETs
        let combined_data_tags = if !additional_cets.is_empty() {
            let mut combined = data_tags.clone();
            combined.extend(additional_cets);
            combined
        } else {
            data_tags
        };

        if let Some(dist_session) = self.state.get_account_session() {
            let request = MediaEventsAccountLinkRequestParams {
                media_event: MediaEvent {
                    content_id: watched_info.entity_id.to_owned(),
                    completed: watched_info.completed.unwrap_or(false),
                    progress: watched_info.progress,
                    progress_unit: request.unit.clone(),
                    watched_on: watched_info.watched_on.clone(),
                    app_id: ctx.app_id.to_owned(),
                },
                content_partner_id: self.get_app_catalog_id(&ctx).await,
                client_supports_opt_out: self.state.get_privacy().allow_watch_history,
                dist_session,
                data_tags: combined_data_tags,
                category_tags: category_tags,
            };
            return XvpPlayback::put_resume_point(
                self.state.config.xvp_playback_service.url.clone(),
                request,
            )
            .await
            .is_ok();
        }
        false
    }

    pub(crate) async fn get_app_catalog_id(&self, ctx: &CallContext) -> String {
        get_app_catalog_id(self.state.clone(), ctx).await
    }
}

pub(crate) async fn get_app_catalog_id(state: DistributorState, ctx: &CallContext) -> String {
    if let Some(mut service_client) = state.get_service_client() {
        if let Ok(result) = service_client
            .request_with_timeout_main(
                "ripple.getAppCatalogId".to_string(),
                None,
                Some(ctx),
                5000,
                EOS_DISTRIBUTOR_SERVICE_ID.to_string(),
            )
            .await
        {
            debug!("receive ripple.getAppCatalogId response: {:?}", result);
            match result.message {
                JsonRpcMessage::Success(v) => {
                    if let Some(result) = v.result.as_str() {
                        debug!("ripple.getAppCatalogId result: {:?}", result);
                        return result.to_owned();
                    }
                }
                _ => {
                    error!(
                        "Failed to get ripple.getAppCatalogId response: {:?}",
                        result
                    );
                }
            }
        }
    } else {
        error!("Service client is unavailable. Cannot get app catalog id.");
    }
    ctx.app_id.clone()
}

fn rpc_downstream_service_err(msg: &str) -> JsonRpcErrorType {
    rpc_error_with_code::<String>(msg, DOWNSTREAM_SERVICE_UNAVAILABLE_ERROR_CODE)
}
