use crate::{
    client::{xvp_playback::XvpPlayback, xvp_videoservice::XvpVideoService},
    gateway::appsanity_gateway::{CloudServiceScopes, GrpcClientSession},
};

use ottx_protos::session_service::{
    AccountLinkAction, AccountLinkType, Entitlement, ImagesData, LinkAccountEntitlementsRequest,
    LinkAccountEntitlementsResponse, LinkAccountLaunchpad, LinkAccountLaunchpadRequest,
    LinkAccountLaunchpadResponse, LinkEntitlements,
};
use std::{
    collections::HashMap,
    sync::{Arc, Mutex},
};
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_discovery::{
    AccountLinkService, DiscoveryEntitlement, EntitlementsAccountLinkRequestParams,
    EntitlementsAccountLinkResponse, LaunchPadAccountLinkRequestParams,
    LaunchPadAccountLinkResponse, MediaEventsAccountLinkRequestParams,
    MediaEventsAccountLinkResponse, SignInRequestParams, ACCOUNT_LINK_ACTION_APP_LAUNCH,
    ACCOUNT_LINK_ACTION_CREATE, ACCOUNT_LINK_ACTION_DELETE, ACCOUNT_LINK_ACTION_SIGN_IN,
    ACCOUNT_LINK_ACTION_SIGN_OUT, ACCOUNT_LINK_TYPE_ACCOUNT_LINK,
    ACCOUNT_LINK_TYPE_ENTITLEMENT_UPDATES, ACCOUNT_LINK_TYPE_LAUNCH_PAD,
};
use thunder_ripple_sdk::ripple_sdk::api::session::AccountSession;
use thunder_ripple_sdk::ripple_sdk::{
    async_trait::async_trait,
    log::{error, info},
    utils::error::RippleError,
};
use tonic::{transport::Channel, Request};

pub struct AppsanityAccountLinkService {
    supported_link_types: HashMap<String, AccountLinkType>,
    supported_link_action: HashMap<String, AccountLinkAction>,
    grpc_client_session: Arc<Mutex<GrpcClientSession>>,
    xvp_service_urls: XvpServiceUrls,
    xvp_data_scopes: CloudServiceScopes,
}
#[derive(Debug, Clone)]
pub struct XvpServiceUrls {
    xvp_playback_base_url: String,
    xvp_video_service_base_url: String,
}

impl XvpServiceUrls {
    pub fn new(xvp_playback_base_url: String, xvp_video_service_base_url: String) -> Self {
        Self {
            xvp_playback_base_url,
            xvp_video_service_base_url,
        }
    }

    pub fn get_xvp_playback_url(&self) -> &str {
        &self.xvp_playback_base_url
    }
    pub fn get_xvp_video_service_url(&self) -> &str {
        &self.xvp_video_service_base_url
    }
}

impl AppsanityAccountLinkService {
    pub fn new(
        grpc_client_session: Arc<Mutex<GrpcClientSession>>,
        xvp_service_urls: XvpServiceUrls,
        xvp_data_scopes: CloudServiceScopes,
    ) -> Self {
        AppsanityAccountLinkService {
            supported_link_types: HashMap::from([
                (
                    ACCOUNT_LINK_TYPE_ACCOUNT_LINK.to_owned(),
                    AccountLinkType::AccountLink,
                ),
                (
                    ACCOUNT_LINK_TYPE_ENTITLEMENT_UPDATES.to_owned(),
                    AccountLinkType::EntitlementsUpdate,
                ),
                (
                    ACCOUNT_LINK_TYPE_LAUNCH_PAD.to_owned(),
                    AccountLinkType::LaunchPad,
                ),
            ]),
            supported_link_action: HashMap::from([
                (
                    ACCOUNT_LINK_ACTION_SIGN_IN.to_owned(),
                    AccountLinkAction::SignIn,
                ),
                (
                    ACCOUNT_LINK_ACTION_SIGN_OUT.to_owned(),
                    AccountLinkAction::SignOut,
                ),
                (
                    ACCOUNT_LINK_ACTION_APP_LAUNCH.to_owned(),
                    AccountLinkAction::AppLaunch,
                ),
                (
                    ACCOUNT_LINK_ACTION_CREATE.to_owned(),
                    AccountLinkAction::Create,
                ),
                (
                    ACCOUNT_LINK_ACTION_DELETE.to_owned(),
                    AccountLinkAction::Delete,
                ),
            ]),
            grpc_client_session,
            xvp_service_urls,
            xvp_data_scopes,
        }
    }

    pub fn get_link_types(&self, key: &String) -> AccountLinkType {
        *self
            .supported_link_types
            .get(key)
            .unwrap_or(&AccountLinkType::AccountLink)
    }

    pub fn get_link_action(&self, key: &String) -> AccountLinkAction {
        *self
            .supported_link_action
            .get(key)
            .unwrap_or(&AccountLinkAction::SignIn) // Todo find out the default.
    }

    pub fn get_entitlements(&self, entitlements: &Vec<DiscoveryEntitlement>) -> Vec<Entitlement> {
        let mut entitlement_map = Vec::new();

        let entitlements_iter = entitlements.iter();
        for item in entitlements_iter {
            entitlement_map.push(Entitlement {
                id: item.entitlement_id.to_owned(),
                start_date: item.start_time as i32,
                end_date: item.end_time as i32,
            })
        }
        entitlement_map
    }

    fn get_images(
        &self,
        images: &HashMap<String, HashMap<String, String>>,
    ) -> HashMap<String, ImagesData> {
        let mut image_map = HashMap::new();
        for (aspect, images_data) in images.iter() {
            for (locale, description) in images_data.iter() {
                image_map.insert(
                    aspect.to_string(),
                    ImagesData {
                        locale: locale.to_string(),
                        image_description: description.to_string(),
                    },
                );
            }
        }
        image_map
    }
}

#[cfg(not(test))]
async fn call_link_account_entitlements(
    channel: Channel,
    perm_req: Request<LinkAccountEntitlementsRequest>,
    dist_session: &AccountSession,
) -> Result<tonic::Response<LinkAccountEntitlementsResponse>, tonic::Status> {
    use crate::util::service_util::decorate_request_with_account_session;

    let mut client =
        ottx_protos::session_service::account_bridge_service_client::AccountBridgeServiceClient::with_interceptor(channel, |mut req: Request<()>| {
            decorate_request_with_account_session(&mut req, dist_session);
            Ok(req)
        });
    return client.link_account_entitlements(perm_req).await;
}

#[cfg(not(test))]
async fn call_link_account_launchpad(
    channel: Channel,
    perm_req: Request<LinkAccountLaunchpadRequest>,
    dist_session: &AccountSession,
) -> Result<tonic::Response<LinkAccountLaunchpadResponse>, tonic::Status> {
    let mut client =
        ottx_protos::session_service::account_bridge_service_client::AccountBridgeServiceClient::with_interceptor(channel, |mut req: Request<()>| {
            crate::util::service_util::decorate_request_with_account_session(&mut req, dist_session);
            Ok(req)
        });
    return client.link_account_launchpad(perm_req).await;
}

#[cfg(test)]
async fn call_link_account_entitlements(
    _channel: Channel,
    _perm_req: Request<LinkAccountEntitlementsRequest>,
    _dist_session: &AccountSession,
) -> Result<tonic::Response<LinkAccountEntitlementsResponse>, tonic::Status> {
    Ok(tonic::Response::new(LinkAccountEntitlementsResponse {}))
}

#[cfg(test)]
async fn call_link_account_launchpad(
    _channel: Channel,
    _perm_req: Request<LinkAccountLaunchpadRequest>,
    _dist_session: &AccountSession,
) -> Result<tonic::Response<LinkAccountLaunchpadResponse>, tonic::Status> {
    Ok(tonic::Response::new(LinkAccountLaunchpadResponse {}))
}

#[async_trait]
impl AccountLinkService for AppsanityAccountLinkService {
    async fn entitlements_account_link(
        self: Box<Self>,
        params: EntitlementsAccountLinkRequestParams,
    ) -> Result<EntitlementsAccountLinkResponse, RippleError> {
        let channel = match self.grpc_client_session.lock().unwrap().get_grpc_channel() {
            Ok(c) => c,
            Err(_e) => return Err(RippleError::ServiceError),
        };
        let perm_req = tonic::Request::new(LinkAccountEntitlementsRequest {
            link_entitlements: Some(LinkEntitlements {
                account_link_type: (params.account_link_type)
                    .map_or_else(|| -1, |x| self.get_link_types(&x) as i32),
                account_link_action: params
                    .account_link_action
                    .map_or_else(|| -1, |x| self.get_link_action(&x) as i32),
                entitlements: self.get_entitlements(&params.entitlements),
                durable_app_id: params.app_id.to_owned(),
            }),
            content_partner_id: params.content_partner_id,
        });
        match call_link_account_entitlements(channel, perm_req, &params.dist_session).await {
            Ok(res) => {
                info!("entitlements_account_link SUCCESSSS !");
                let _res_i = res.into_inner();
                let response = EntitlementsAccountLinkResponse {};
                Ok(response)
            }
            Err(e) => {
                error!("Error Notifying Entitlements: {:?}", e);
                Err(RippleError::ServiceError)
            }
        }
    }
    async fn media_events_account_link(
        self: Box<Self>,
        params: MediaEventsAccountLinkRequestParams,
    ) -> Result<MediaEventsAccountLinkResponse, RippleError> {
        match XvpPlayback::put_resume_point(
            self.xvp_service_urls.get_xvp_playback_url().to_string(),
            params,
        )
        .await
        {
            Ok(_) => Ok(MediaEventsAccountLinkResponse {}),
            Err(_) => Err(RippleError::ServiceError),
        }
    }
    async fn launch_pad_account_link(
        self: Box<Self>,
        params: LaunchPadAccountLinkRequestParams,
    ) -> Result<LaunchPadAccountLinkResponse, RippleError> {
        let channel = match self.grpc_client_session.lock().unwrap().get_grpc_channel() {
            Ok(c) => c,
            Err(_e) => {
                return Err(RippleError::ServiceError);
            }
        };
        let perm_req = tonic::Request::new(LinkAccountLaunchpadRequest {
            link_launchpad: Some(LinkAccountLaunchpad {
                expiration: params.link_launchpad.expiration,
                app_name: params.link_launchpad.app_name.to_owned(),
                content_id: if params.link_launchpad.content_id.is_some() {
                    params
                        .link_launchpad
                        .content_id
                        .as_ref()
                        .unwrap()
                        .to_string()
                } else {
                    "".to_owned()
                },
                deeplink: if params.link_launchpad.deeplink.is_some() {
                    params.link_launchpad.deeplink.as_ref().unwrap().to_string()
                } else {
                    "".to_owned()
                },
                content_url: if params.link_launchpad.content_url.is_some() {
                    params
                        .link_launchpad
                        .content_url
                        .as_ref()
                        .unwrap()
                        .to_string()
                } else {
                    "".to_owned()
                },
                durable_app_id: params.link_launchpad.app_id.to_owned(),
                title: params.link_launchpad.title.clone(),
                images: self.get_images(&params.link_launchpad.images),
                account_link_type: self.get_link_types(&params.link_launchpad.account_link_type)
                    as i32,
                account_link_action: self
                    .get_link_action(&params.link_launchpad.account_link_action)
                    as i32,
            }),
            content_partner_id: params.content_partner_id,
        });

        match call_link_account_launchpad(channel, perm_req, &params.dist_session).await {
            Ok(_res) => {
                info!("launch_pad_account_link SUCCESSSS !");
                //let response = res.into_inner();
                let response = LaunchPadAccountLinkResponse {};
                Ok(response)
            }
            Err(e) => {
                error!("Error Notifying launch_pad_account_link : {:?}", e);
                Err(RippleError::ServiceError)
            }
        }
    }
    async fn sign_in(self: Box<Self>, params: SignInRequestParams) -> Result<(), RippleError> {
        match XvpVideoService::sign_in(
            self.xvp_service_urls.get_xvp_video_service_url(),
            self.xvp_data_scopes.get_xvp_sign_in_state_scope(),
            params,
        )
        .await
        {
            Ok(_) => {
                info!("XVP SignIn returned success");
                Ok(())
            }
            Err(_) => {
                error!("Error in sending SignIn Information");
                Err(RippleError::ServiceError)
            }
        }
    }
}

#[cfg(test)]
pub mod tests {
    use crate::{
        client::xvp_playback::ResumePointResponse,
        client::xvp_videoservice::XvpVideoServiceResponse,
        gateway::appsanity_gateway::CloudServiceScopes,
        service::appsanity_account_link::{AppsanityAccountLinkService, XvpServiceUrls},
    };

    use crate::util::service_util::create_grpc_client_session;

    use httpmock::{Method::PUT, MockServer};
    use std::collections::{HashMap, HashSet};
    use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_discovery::{
        AccountLaunchpad, AccountLinkService, DiscoveryEntitlement,
        EntitlementsAccountLinkRequestParams, LaunchPadAccountLinkRequestParams, MediaEvent,
        MediaEventsAccountLinkRequestParams, ProgressUnit, SessionParams, SignInRequestParams,
        ACCOUNT_LINK_ACTION_CREATE, ACCOUNT_LINK_TYPE_ENTITLEMENT_UPDATES,
        ACCOUNT_LINK_TYPE_LAUNCH_PAD,
    };
    use thunder_ripple_sdk::ripple_sdk::api::session::AccountSession;

    fn get_service(url: String) -> Box<AppsanityAccountLinkService> {
        let grpc_client_session =
            create_grpc_client_session(String::from("res-api.svc-qa.thor.comcast.com"));
        Box::new(AppsanityAccountLinkService::new(
            grpc_client_session,
            XvpServiceUrls::new(url.clone(), url.clone()),
            CloudServiceScopes::default(),
        ))
    }

    #[tokio::test]
    pub async fn test_entitlements_account_link() {
        let dist_session = AccountSession {
            id: String::from("xglobal"),
            token: String::from("RmlyZWJvbHQgTWFuYWdlIFNESyBSb2NrcyEhIQ=="),
            account_id: String::from("0123456789"),
            device_id: String::from("9876543210"),
        };

        let request_params = EntitlementsAccountLinkRequestParams {
            account_link_type: Some(ACCOUNT_LINK_TYPE_ENTITLEMENT_UPDATES.to_owned()),
            account_link_action: None,
            entitlements: vec![DiscoveryEntitlement {
                entitlement_id: String::from("123"),
                start_time: 1735689600,
                end_time: 1735689600,
            }],
            app_id: String::from("ABCDEF"),
            content_partner_id: String::from("STUVW"),
            dist_session,
        };

        let service = get_service(String::from("https://example.com"));
        let result = service.entitlements_account_link(request_params).await;
        assert!(result.is_ok());
    }

    #[tokio::test]
    pub async fn test_media_events_account_link() {
        let dist_session = AccountSession {
            id: String::from("xglobal"),
            token: String::from("RmlyZWJvbHQgTWFuYWdlIFNESyBSb2NrcyEhIQ=="),
            account_id: String::from("0123456789"),
            device_id: String::from("9876543210"),
        };
        let request_params = MediaEventsAccountLinkRequestParams {
            media_event: MediaEvent {
                content_id: String::from("partner.com/entity/123"),
                completed: true,
                progress: 50.0,
                progress_unit: Some(ProgressUnit::Percent),
                watched_on: Some(String::from("2021-04-23T18:25:43.511Z")),
                app_id: String::from("ABCDEF"),
            },
            content_partner_id: String::from("STUVW"),
            client_supports_opt_out: false,
            data_tags: HashSet::default(),
            dist_session,
            category_tags: vec!["tag1".into(), "tag2".into()],
        };
        let server = MockServer::start();
        let resp = ResumePointResponse {
            message_id: Some(String::from("mid")),
            sns_status_code: Some(200),
            sns_status_text: Some(String::from("text")),
            aws_request_id: Some(String::from("arid")),
        };

        let xvp_mock = server.mock(|when, then| {
            when.method(PUT).path("/base_url/v1/partners/xglobal/accounts/0123456789/devices/9876543210/resumePoints/ott/STUVW/partner.com%2Fentity%2F123").query_param("clientId", "ripple");
            then.status(200)
                .header("content-type", "application/javascript")
                .body(serde_json::to_string(&resp).unwrap());
        });

        let service = get_service(server.url("/base_url/v1"));
        let result = service.media_events_account_link(request_params).await;
        assert!(result.is_ok());
        xvp_mock.assert();
    }

    #[tokio::test]
    pub async fn test_launch_pad_account_link() {
        let server = MockServer::start();
        let dist_session = AccountSession {
            id: String::from("xglobal"),
            token: String::from("RmlyZWJvbHQgTWFuYWdlIFNESyBSb2NrcyEhIQ=="),
            account_id: String::from("0123456789"),
            device_id: String::from("9876543210"),
        };
        let request_params = LaunchPadAccountLinkRequestParams {
            link_launchpad: AccountLaunchpad {
                expiration: 1735689600,
                app_name: String::from("Netflix"),
                content_id: Some(String::from("partner.com/entity/123")),
                deeplink: None,
                content_url: None,
                app_id: String::from("ABCDEF"),
                title: HashMap::from([(String::from("en"), String::from("Test Description"))]),
                images: HashMap::new(),
                account_link_type: ACCOUNT_LINK_TYPE_LAUNCH_PAD.to_owned(),
                account_link_action: ACCOUNT_LINK_ACTION_CREATE.to_owned(),
            },
            content_partner_id: String::from("STUVW"),
            dist_session,
        };

        let service = get_service(server.url("/base_url/v1"));
        let result = service.launch_pad_account_link(request_params).await;
        assert!(result.is_ok());
    }

    #[tokio::test]
    pub async fn test_sign_in() {
        let dist_session = AccountSession {
            id: String::from("app1"),
            token: String::from("RmlyZWJvbHQgTWFuYWdlIFNESyBSb2NrcyEhIQ=="),
            account_id: String::from("0123456789"),
            device_id: String::from("9876543210"),
        };
        let request_params = SignInRequestParams {
            session_info: SessionParams {
                app_id: String::from("app1"),
                dist_session,
            },
            is_signed_in: true,
        };

        let server = MockServer::start();
        let resp = XvpVideoServiceResponse {
            partner_id: Some("app1".to_owned()),
            account_id: Some("0123456789".to_owned()),
            owner_reference: Some("xrn:xcal:subscriber:account:0123456789".to_owned()),
            entity_urn: Some("xrn:xvp:application:app1".to_owned()),
            entity_id: Some("app1".to_owned()),
            entity_type: Some("application".to_owned()),
            durable_app_id: Some("app1".to_owned()),
            event_type: Some("signIn".to_owned()),
            is_signed_in: Some(true),
            added: Some("2023-03-09T23:01:06.397Z".to_owned()),
            updated: Some("2023-03-13T17:32:20.610478364Z".to_owned()),
        };

        let xvp_session_mock = server.mock(|when, then| {
            when.method(PUT).path("/base_url/v1/partners/app1/accounts/0123456789/videoServices/xrn:xvp:application:app1/engaged")
            .query_param("ownerReference", "xrn:xcal:subscriber:account:0123456789")
            .query_param("clientId", "ripple");
            then.status(200)
                .header("content-type", "application/javascript")
                .body(serde_json::to_string(&resp).unwrap());
        });

        let service = get_service(server.url("/base_url/v1"));
        let result = service.sign_in(request_params).await;
        assert!(result.is_ok());
        xvp_session_mock.assert();
    }
}
