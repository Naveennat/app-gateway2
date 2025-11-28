use crate::{
    client::xvp_xifaservice::{XifaParams, XvpXifaService},
    gateway::appsanity_gateway::GrpcClientSession,
    message::DpabError,
    util::service_util::decorate_request_with_account_session,
};

use base64::{engine::general_purpose::STANDARD as base64, Engine};
use ottx_protos::ad_platform::{
    ad_platform_service_client::AdPlatformServiceClient, AdRouterRequest,
};
use thunder_ripple_sdk::ripple_sdk::api::firebolt::fb_advertising::{
    AdConfigRequestParams, AdConfigResponse, AdIdRequestParams, AdIdResponse,
};
use thunder_ripple_sdk::ripple_sdk::api::session::AccountSession;
use thunder_ripple_sdk::ripple_sdk::log::{debug, error};

use serde::Serialize;
use serde_json::{to_string, Error};
use std::{
    collections::HashMap,
    sync::{Arc, Mutex},
};
use tonic::Request;

#[derive(Clone)]
pub struct AppsanityAdvertisingService {
    pub grpc_client_session: Arc<Mutex<GrpcClientSession>>,
    pub xvp_service_url: XvpServiceUrl,
}

#[derive(Debug, Clone)]
pub struct XvpServiceUrl {
    pub xvp_xifa_url: String,
}

impl XvpServiceUrl {
    pub fn new(xvp_xifa_url: String) -> Self {
        Self { xvp_xifa_url }
    }

    pub fn get_xvp_xifa_url(&self) -> &str {
        &self.xvp_xifa_url
    }
}

impl AppsanityAdvertisingService {
    pub fn new(
        grpc_client_session: Arc<Mutex<GrpcClientSession>>,
        xvp_service_url: XvpServiceUrl,
    ) -> Self {
        AppsanityAdvertisingService {
            grpc_client_session,
            xvp_service_url,
        }
    }
}

impl AppsanityAdvertisingService {
    pub async fn get_ad_identifier(
        self: Self,
        params: AdIdRequestParams,
    ) -> Result<AdIdResponse, DpabError> {
        match XvpXifaService::get_ad_identifier(
            self.xvp_service_url.get_xvp_xifa_url(),
            XifaParams {
                privacy_data: params.privacy_data.clone(),
                app_id: Some(params.app_id.clone()),
                dist_session: params.dist_session.clone(),
                scope: params.scope.clone(),
            },
        )
        .await
        {
            Ok(res) => {
                let response = AdIdResponse {
                    ifa: res.xifa,
                    ifa_type: res.xifa_type,
                    lmt: params
                        .privacy_data
                        .get("lmt")
                        .cloned()
                        .unwrap_or("1".to_owned()),
                };
                Ok(response)
            }
            Err(e) => {
                error!("Error getting xifa {:?}", e);
                Err(DpabError::IoError)
            }
        }
    }

    pub async fn reset_ad_identifier(self: Self, params: AccountSession) -> Result<(), DpabError> {
        match XvpXifaService::reset_ad_identifier(
            self.xvp_service_url.get_xvp_xifa_url(),
            XifaParams {
                privacy_data: HashMap::new(),
                app_id: None,
                dist_session: params.clone(),
                scope: HashMap::new(),
            },
        )
        .await
        {
            Ok(res) => {
                debug!("XIFA reset successful received response {:?}", res);
                Ok(())
            }
            Err(e) => {
                error!("Error resetting xifa: {:?}", e);
                Err(DpabError::IoError)
            }
        }
    }

    pub async fn get_ad_config(
        self: Self,
        params: AdConfigRequestParams,
    ) -> Result<AdConfigResponse, DpabError> {
        let mut config = AdConfigResponse::default();
        config.app_bundle_id = format!("{}.{}", params.durable_app_id, "Comcast");

        match XvpXifaService::get_ad_identifier(
            self.xvp_service_url.get_xvp_xifa_url(),
            XifaParams {
                privacy_data: params.privacy_data.clone(),
                app_id: Some(params.durable_app_id.clone()),
                dist_session: params.dist_session.clone(),
                scope: params.scope.clone(),
            },
        )
        .await
        {
            Ok(resp) => {
                let lmt = params
                    .privacy_data
                    .get("lmt")
                    .cloned()
                    .unwrap_or("1".to_owned());
                match serialize_object(AdIdResponse {
                    ifa: resp.xifa.clone(),
                    ifa_type: resp.xifa_type,
                    lmt,
                }) {
                    Ok(ifa_enc) => {
                        config.ifa = ifa_enc;
                        config.ifa_value = resp.xifa;
                    }
                    Err(e) => {
                        error!("get_ad_config: Failed to serialize ifa: e={:?}", e);
                    }
                }
            }
            Err(e) => error!("get_ad_config: Failed to get xfia: e={:?}", e),
        }

        let channel = self.grpc_client_session.lock().unwrap().get_grpc_channel();

        if let Err(e) = channel {
            error!("get_ad_config: Failed to get channel: e={:?}", e);
            return Err(DpabError::IoError);
        }

        let mut client =
            AdPlatformServiceClient::with_interceptor(channel.unwrap(), |mut req: Request<()>| {
                decorate_request_with_account_session(&mut req, &params.dist_session);
                Ok(req)
            });

        let perm_req = tonic::Request::new(AdRouterRequest {
            durable_app_id: params.durable_app_id,
            environment: params.environment,
        });

        debug!("get_ad_config: perm_req={:?}", perm_req);

        match client.get_ad_router(perm_req).await {
            Ok(res) => {
                let res_i = res.into_inner();
                config.ad_server_url = res_i.ad_server_url;
                config.ad_server_url_template = res_i.ad_server_url_template;
                config.ad_network_id = res_i.ad_network_i_d;
                config.ad_profile_id = res_i.ad_profile_i_d;
                config.ad_site_section_id = res_i.ad_site_section_i_d;
            }
            Err(e) => {
                error!("get_ad_config: Failed to get adRouter: e={:?}", e);
            }
        }
        Ok(config)
    }
}

pub fn serialize_object<T: Serialize>(obj: T) -> Result<String, Error> {
    let json_string = to_string(&obj)?;
    let encoded = base64.encode(json_string.as_bytes());
    Ok(encoded)
}

#[allow(unused)]
#[cfg(test)]
pub mod tests {

    use crate::client::xvp_xifaservice::XvpXifaServiceResponse;
    use crate::service::appsanity_advertising::{AppsanityAdvertisingService, XvpServiceUrl};
    use crate::util::service_util::create_grpc_client_session;
    use httpmock::prelude::*;
    use std::collections::HashMap;
    use thunder_ripple_sdk::ripple_sdk::api::{
        firebolt::fb_advertising::{AdConfigRequestParams, AdIdRequestParams},
        session::AccountSession,
    };

    fn get_session() -> AccountSession {
        AccountSession {
            id: String::from("id_1"),
            token: String::from("token_adveristing"),
            account_id: String::from("account_id_1"),
            device_id: String::from("device_id_1"),
        }
    }

    fn get_service(url: String) -> Box<AppsanityAdvertisingService> {
        let grpc_client_session =
            create_grpc_client_session(String::from("ad-platform-service.svc-qa.thor.comcast.com"));
        Box::new(AppsanityAdvertisingService::new(
            grpc_client_session,
            XvpServiceUrl::new(url),
        ))
    }

    fn get_xifa_response() -> XvpXifaServiceResponse {
        XvpXifaServiceResponse {
            partner_id: Some("id_1".to_owned()),
            account_id: Some("account_id_1".to_owned()),
            device_id: Some(String::from("device_id_1")),
            profile_id: Some(String::from("abcd")),
            xifa: String::from("1234"),
            xifa_type: String::from("A"),
            entity_scope_ids: None,
            expiration: None,
            match_rule: None,
        }
    }

    pub fn mock_xifa_response(server: &MockServer) -> httpmock::Mock<'_> {
        let resp = get_xifa_response();
        let xvp_mock = server.mock(|when, then| {
                when.method(GET).path("/base_url/v1/partners/id_1/accounts/account_id_1/devices/device_id_1/autoresolve/xifa")
                .query_param("clientId", "ripple");
                then.status(200)
                    .header("content-type", "application/javascript")
                    .body(serde_json::to_string(&resp).unwrap());
            });
        return xvp_mock;
    }

    #[tokio::test]
    pub async fn test_get_ad_id_object() {
        let mut passed = false;
        let server = MockServer::start();
        let xvp_mock = mock_xifa_response(&server);
        let ad_id_request_params = AdIdRequestParams {
            privacy_data: HashMap::new(),
            app_id: String::from("App_id_1"),
            dist_session: get_session(),
            scope: HashMap::new(),
        };

        let service = get_service(server.url("/base_url/v1"));
        let result = service.get_ad_identifier(ad_id_request_params).await;
        passed = result.is_ok();
        assert!(passed);
        xvp_mock.assert();
    }

    #[tokio::test]
    pub async fn test_reset_ad_id() {
        let mut passed = false;
        let server = MockServer::start();
        let resp = [get_xifa_response()];
        let xvp_mock = server.mock(|when, then| {
            when.method(POST)
                .path("/base_url/v1/partners/id_1/accounts/account_id_1/xifas/reset")
                .query_param("clientId", "ripple");
            then.status(200)
                .header("content-type", "application/javascript")
                .body(serde_json::to_string(&resp).unwrap());
        });

        let service = get_service(server.url("/base_url/v1"));
        let result = service.reset_ad_identifier(get_session()).await;
        passed = result.is_ok();
        assert!(passed);
        xvp_mock.assert();
    }
    #[cfg(feature = "broken_tests")]
    #[tokio::test]
    pub async fn test_get_ad_config() {
        // This test is broken because the mock server is somehow not being used by the test
        //evidenced by log messages like this:
        /*
        [2025-03-14T13:39:57Z ERROR eos_distributor::service::appsanity_advertising] get_ad_config: Failed to get adRouter:
        e=Status { code: Unauthenticated, message: "Unable to parse jwt from string. token_adveristing", metadata: MetadataMap
        { headers: {"content-type": "application/grpc", "authorization": "Bearer token_adveristing", "deviceid": "device_id_1",
         "accountid": "account_id_1", "partnerid": "id_1", "x-forwarded-for": "172.31.73.75", "x-forwarded-proto": "https",
         "x-request-id": "f05e58df-26dc-4e03-bfdc-199e3b760c42", "x-envoy-attempt-count": "1",
         "x-envoy-internal": "true",
         "x-forwarded-client-cert": "By=spiffe://cluster.local/ns/ott-platform-qa/sa/default;Hash=f78945fb88111fde60bdb5e6f6ab780d11f9fac72d4ddf81295048c75dacd652;Subject=\"\";URI=spiffe://cluster.local/ns/istio-system/sa/istio-ingressgateway-service-account", "server": "istio-envoy", "date": "Fri, 14 Mar 2025 13:39:57 GMT", "x-envoy-upstream-service-time": "10"} }, source: None }
         */
        let mut passed = false;
        let server = MockServer::start();
        let xvp_mock = mock_xifa_response(&server);
        let mut privacy_data = HashMap::new();
        privacy_data.insert(String::from("app_id"), String::from("device_id"));

        let ad_config_request_params = AdConfigRequestParams {
            privacy_data,
            durable_app_id: String::from("durable_app_id"),
            dist_session: get_session(),
            environment: String::from("environment"),
            scope: HashMap::new(),
        };

        let service = get_service(server.url("/base_url/v1"));
        let result = service.get_ad_config(ad_config_request_params).await;
        passed = result.is_ok();
        assert!(passed);
        xvp_mock.assert();
    }
}
