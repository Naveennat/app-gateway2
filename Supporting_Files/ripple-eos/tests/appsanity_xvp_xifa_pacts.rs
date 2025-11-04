use crate::client::xvp_xifaservice::{
    XifaOperation, XifaParams, XvpXifaService, XvpXifaServiceErrResponse,
};
use crate::util::http_client::{HttpClient, HttpError};
use pact_consumer::prelude::*;
use pact_consumer::*;
use ripple_sdk::{
    api::session::AccountSession,
    log::{debug, error},
};
use std::collections::HashMap;

#[tokio::test(flavor = "multi_thread")]
#[cfg_attr(not(feature = "http_contract_tests"), ignore)]
async fn test_get_xifa_contract() {
    let pact = PactBuilder::new("ripple", "xifa-service")
        .interaction("a request to acquire a XIFA", "", |mut i| {
            i.given("the XIFA service is up and running");
            i.request.method("GET");
            i.request.path(term!("/partners/comcast/accounts/1234512345/devices/67896789/autoresolve/xifa", "^/partners/([A-Za-z])([a-zA-Z0-9-_]+)/accounts/([0-9]+)/devices/([0-9]+)/autoresolve/xifa$"));
            i.request.content_type("application/json");
            i.request.header("Authorization", term!("^Bearer [a-zA-Z0-9_.=]*$", "Bearer eyJraWQiOiJzYXQtcHJvZC1rMS0xMDI0IiwiYWxnIjoiUlMyNTYifQ.eyJwcmluY_lwYWwiOnsiZGdkIjpbTVEsMm0IMoXbF2mLrJZ6DmEQf7_YAe9U=="));

            i.request.query_param("adTrackingParticipationState", term!("^(true|false)$", "false"));
            i.request.query_param("clientId", term!("^([A-Za-z])([a-zA-Z0-9-_]+)$", "ripple"));
            i.request.query_param("entityScopeId", term!("^([a-zA-Z0-9-:_.]+)$", "scope_id"));

            i.response
                .content_type("application/json")
                .status(200)
                .json_body(json_pattern!({
                    "partnerId": like!("string"),
                    "accountId": like!("string"),
                    "deviceId": like!("string"),
                    "profileId": like!(r#"^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"#),
                    "xifa": like!(r#"^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"#),
                    "xifaType": term!("sspid|sessionId", "sspid"),
                    "entityScopeIds": ["string"],
                    "matchRule": {
                        "matchRuleId": like!(r#"^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"#),
                        "matchRuleVersion": like!(0)
                    },
                    "expiration": like!("string")
                }));
            i
        })
        .start_mock_server(None);

    let xifa_url = pact.path("");
    let distributor_session = AccountSession {
        id: String::from("comcast"),
        token: String::from("token_id"),
        account_id: String::from("1234512345"),
        device_id: String::from("67896789"),
    };
    let result = XvpXifaService::get_ad_identifier(
        &xifa_url.to_string(),
        XifaParams {
            privacy_data: HashMap::new(),
            app_id: Some("comcast_firebolt_reference".to_string()),
            dist_session: distributor_session,
            scope: HashMap::new(),
        },
    )
    .await;
    assert!(result.is_ok(), "Failed to get XIFA: {:?}", result.err());
}

#[tokio::test(flavor = "multi_thread")]
#[cfg_attr(not(feature = "http_contract_tests"), ignore)]
async fn test_reset_xifa_contract() {
    let pact = PactBuilder::new("ripple", "xifa-service")
        .interaction("a request to reset XIFA", "", |mut i| {
            i.given("the XIFA service is up and running");
            i.request.method("POST");
            i.request.path(term!("/partners/comcast/accounts/1234512345/xifas/reset", "^/partners/([A-Za-z])([a-zA-Z0-9-_]+)/accounts/([0-9]+)/xifas/reset$"));
            i.request.content_type("application/json");
            i.request.header("Authorization", term!("^Bearer [a-zA-Z0-9_.=]*$", "Bearer eyJraWQiOiJzYXQtcHJvZC1rMS0xMDI0IiwiYWxnIjoiUlMyNTYifQ.eyJwcmluY_lwYWwiOnsiZGdkIjpbTVEsMm0IMoXbF2mLrJZ6DmEQf7_YAe9U=="));

            i.request.query_param("clientId", term!("^([A-Za-z])([a-zA-Z0-9-_]+)$", "ripple"));
            i.request.query_param("entityScopeId", term!("^([a-zA-Z0-9-:_.]+)$", "scope_id"));

            i.response
                .content_type("application/json")
                .status(200)
                .json_body(json_pattern!([
                    {
                        "partnerId": like!("string"),
                        "accountId": like!("string"),
                        "deviceId": like!("string"),
                        "profileId": like!(r#"^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"#),
                        "xifa": like!(r#"^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"#),
                        "xifaType": term!("sspid|sessionId", "sspid"),
                        "entityScopeIds": ["string"],
                        "matchRule": {
                            "matchRuleId": like!(r#"^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"#),
                            "matchRuleVersion": like!(0)
                        },
                        "expiration": like!("string")
                    }
                ]));
            i
        })
        .start_mock_server(None);

    let xifa_url = pact.path("");
    let distributor_session = AccountSession {
        id: String::from("comcast"),
        token: String::from("token_id"),
        account_id: String::from("1234512345"),
        device_id: String::from("67896789"),
    };
    let params = XifaParams {
        privacy_data: HashMap::new(),
        app_id: Some("comcast_firebolt_reference".to_string()),
        dist_session: distributor_session,
        scope: HashMap::new(),
    };
    let result = XvpXifaService::reset_ad_identifier(&xifa_url.to_string(), params).await;
    assert!(result.is_ok(), "Failed to reset XIFA: {:?}", result.err());
}

#[tokio::test(flavor = "multi_thread")]
#[cfg_attr(not(feature = "http_contract_tests"), ignore)]
async fn test_get_xifa_contract_error_handling() {
    let error_codes = vec![400, 401, 403, 404, 500, 502, 503];
    for error_code in error_codes {
        let pact = PactBuilder::new("ripple", "xifa-service")
        .interaction(
            format!("a request to acquire a XIFA  when the service has (status: {})", error_code).to_string(),
            "".to_string(),
            |mut i| {
            i.given("the XIFA service is down");
            i.request.method("GET");
            i.request.path(term!("/partners/comcast/accounts/1234512345/devices/67896789/autoresolve/xifa", "^/partners/([A-Za-z])([a-zA-Z0-9-_]+)/accounts/([0-9]+)/devices/([0-9]+)/autoresolve/xifa$"));
            i.request.content_type("application/json");
            i.request.header("Authorization", term!("^Bearer [a-zA-Z0-9_.=]*$", "Bearer eyJraWQiOiJzYXQtcHJvZC1rMS0xMDI0IiwiYWxnIjoiUlMyNTYifQ.eyJwcmluY_lwYWwiOnsiZGdkIjpbTVEsMm0IMoXbF2mLrJZ6DmEQf7_YAe9U=="));
            i.request.query_param("adTrackingParticipationState", term!("^(true|false)$", "false"));
            i.request.query_param("clientId", term!("^([A-Za-z])([a-zA-Z0-9-_]+)$", "ripple"));
            i.request.query_param("entityScopeId", term!("^([a-zA-Z0-9-:_.]+)$", "scope_id"));
            i.response
                .content_type("application/problem+json")
                .status(error_code)
                .json_body(json_pattern!({
                "type": like!("string"),
                "title": like!("string"),
                "status": 599,
                "detail": like!("string"),
                "instance": like!("string"),
            }));
                i
            })
            .start_mock_server(None);

        let pact_url = pact.path("");

        let distributor_session = AccountSession {
            id: String::from("comcast"),
            token: String::from("token_id"),
            account_id: String::from("1234512345"),
            device_id: String::from("67896789"),
        };

        let params = XifaParams {
            privacy_data: HashMap::new(),
            app_id: Some("comcast_firebolt_reference".to_string()),
            dist_session: distributor_session,
            scope: HashMap::new(),
        };

        let xifa_url =
            XvpXifaService::build_xifa_url(&pact_url.to_string(), &params, XifaOperation::Get)
                .unwrap();

        let result: Result<XvpXifaServiceErrResponse, HttpError> =
            xvp_xifa_request_client("GET", xifa_url, params.dist_session.token).await;
        if let Ok(error_response) = result {
            if let Some(_type) = error_response._type {
                assert_eq!(_type, "string");
            } else {
                panic!("'_type' field is missing in the error response");
            }

            if let Some(title) = error_response.title {
                assert_eq!(title, "string");
            } else {
                panic!("'title' field is missing in the error response");
            }

            if let Some(status) = error_response.status {
                assert_eq!(status, 599);
            } else {
                panic!("'status' field is missing in the error response");
            }

            if let Some(detail) = error_response.detail {
                assert_eq!(detail, "string");
            } else {
                panic!("'detail' field is missing in the error response");
            }

            if let Some(instance) = error_response.instance {
                assert_eq!(instance, "string");
            } else {
                panic!("'instance' field is missing in the error response");
            }
        } else {
            panic!(
                "Expected error with status code {}, but got a successful result",
                error_code
            );
        }
    }
}

#[tokio::test(flavor = "multi_thread")]
#[cfg_attr(not(feature = "http_contract_tests"), ignore)]
async fn test_reset_xifa_contract_error_handling() {
    let error_codes = vec![400, 401, 403, 404, 500, 502, 503];
    for error_code in error_codes {
        let pact = PactBuilder::new("ripple", "xifa-service")
        .interaction(
            format!("a request to reset XIFA when the service has (status: {})", error_code).to_string(),
            "".to_string(),
            |mut i| {
            i.given("the XIFA service is down");
            i.request.method("POST");
            i.request.path(term!("/partners/comcast/accounts/1234512345/xifas/reset", "^/partners/([A-Za-z])([a-zA-Z0-9-_]+)/accounts/([0-9]+)/xifas/reset$"));
            i.request.content_type("application/json");
            i.request.header("Authorization", term!("^Bearer [a-zA-Z0-9_.=]*$", "Bearer eyJraWQiOiJzYXQtcHJvZC1rMS0xMDI0IiwiYWxnIjoiUlMyNTYifQ.eyJwcmluY_lwYWwiOnsiZGdkIjpbTVEsMm0IMoXbF2mLrJZ6DmEQf7_YAe9U=="));

            i.request.query_param("clientId", term!("^([A-Za-z])([a-zA-Z0-9-_]+)$", "ripple"));
            i.request.query_param("entityScopeId", term!("^([a-zA-Z0-9-:_.]+)$", "scope_id"));

            i.response
                .content_type("application/problem+json")
                .status(error_code)
                .json_body(json_pattern!({
                    "type": like!("string"),
                    "title": like!("string"),
                    "status": 599,
                    "detail": like!("string"),
                    "instance": like!("string"),
                }));
                    i
                })
                .start_mock_server(None);
        let pact_url = pact.path("");

        let distributor_session = AccountSession {
            id: String::from("comcast"),
            token: String::from("token_id"),
            account_id: String::from("1234512345"),
            device_id: String::from("67896789"),
        };

        let params = XifaParams {
            privacy_data: HashMap::new(),
            app_id: Some("comcast_firebolt_reference".to_string()),
            dist_session: distributor_session,
            scope: HashMap::new(),
        };

        let xifa_url =
            XvpXifaService::build_xifa_url(&pact_url.to_string(), &params, XifaOperation::Reset)
                .unwrap();
        let result: Result<XvpXifaServiceErrResponse, HttpError> =
            xvp_xifa_request_client("POST", xifa_url, params.dist_session.token).await;

        if let Ok(error_response) = result {
            if let Some(_type) = error_response._type {
                assert_eq!(_type, "string");
            } else {
                panic!("'_type' field is missing in the error response");
            }

            if let Some(title) = error_response.title {
                assert_eq!(title, "string");
            } else {
                panic!("'title' field is missing in the error response");
            }

            if let Some(status) = error_response.status {
                assert_eq!(status, 599);
            } else {
                panic!("'status' field is missing in the error response");
            }

            if let Some(detail) = error_response.detail {
                assert_eq!(detail, "string");
            } else {
                panic!("'detail' field is missing in the error response");
            }

            if let Some(instance) = error_response.instance {
                assert_eq!(instance, "string");
            } else {
                panic!("'instance' field is missing in the error response");
            }
        } else {
            panic!(
                "Expected error with status code {}, but got a successful result",
                error_code
            );
        }
    }
}

async fn xvp_xifa_request_client<T>(
    method: &str,
    uri: String,
    auth_token: String,
) -> Result<T, HttpError>
where
    T: serde::de::DeserializeOwned + std::fmt::Debug,
{
    let mut http = HttpClient::new();
    http.set_token(auth_token);
    let response = match method {
        "POST" => http.post(uri.into(), String::default()).await,
        _ => http.get(uri.into(), String::default()).await,
    };

    if let Ok(body_string) = response {
        let result: Result<T, serde_json::Error> = serde_json::from_str(&body_string);
        return result.map_err(|e| {
            error!("XvpXifaServiceResponse parse error {:?}", e);
            HttpError::ServiceError
        });
    } else {
        let body_string = http.get_error_body();
        let response: Result<T, serde_json::Error> = serde_json::from_str(&body_string);
        error!(
            "xvp_xifa returned a non-succesful status: body: {:?}",
            body_string
        );
        if let Ok(r) = response {
            debug!("xvp-xifa error response: {:#?}", r);
            return Ok(r);
        }
    }

    Err(HttpError::ServiceError)
}
