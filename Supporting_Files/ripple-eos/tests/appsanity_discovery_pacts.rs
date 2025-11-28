use crate::client::xvp_session::XvpSession;
use pact_consumer::prelude::*;
use pact_consumer::*;
use ripple_sdk::api::firebolt::fb_discovery::{
    ClearContentSetParams, ContentAccessAvailability, ContentAccessEntitlement, ContentAccessInfo,
    ContentAccessListSetParams, SessionParams,
};
use ripple_sdk::api::session::AccountSession;

#[tokio::test(flavor = "multi_thread")]
#[cfg_attr(not(feature = "http_contract_tests"), ignore)]
async fn test_set_content_access_with_availabilities_entitlements_contract() {
    let xvp_service = PactBuilder::new("ripple", "xvp-service")
    .interaction("a request to set content access with availabilities and entitlements", "", |mut i| {
        i.given("content access service has been up and running");
        i.request.method("PUT");
        i.request.path(term!("^/partners/[a-zA-Z0-9]*/accounts/[0-9]*/appSettings/[a-zA-Z0-9]*$", "/partners/comcast/accounts/1234512345/appSettings/refui"));
        i.request.content_type("application/json");
        i.request.header("authorization", term!("^Bearer [a-zA-Z0-9_.=]*$", "Bearer eyJraWQiOiJzYXQtcHJvZC1rMS0xMDI0IiwiYWxnIjoiUlMyNTYifQ.eyJwcmluY_lwYWwiOnsiZGdkIjpbTVEsMm0IMoXbF2mLrJZ6DmEQf7_YAe9U=="));
        i.request.json_body(json_pattern!(
            {
                "availabilities": [
                    {
                        "type": like!("channel-lineup"),
                        "id": like!("partner.com/availability/123"),
                        "startTime": datetime!("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'","2021-04-23T18:25:43.511Z"),
                        "endTime": datetime!("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'","2021-04-23T18:25:43.511Z"),
                        "catalogId": like!("catalog_id")
                    }
                ],
                "entitlements": [
                    {
                        "entitlementId": like!("test-entitlement-firecert-1"),
                        "startTime": datetime!("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'","2021-04-23T18:25:43.511Z"),
                        "endTime": datetime!("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'","2021-04-23T18:25:43.511Z")
                    }
                ]
            }
        ));
        i.request.query_param("deviceId", term!("^[0-9]*$", "123"));
        i.request.query_param("clientId", term!("^[0-9a-zA-Z]*$", "ripple"));
        i.response
            .content_type("application/json")
            .status(200);
        i}).start_mock_server(None);

    let xvp_url = xvp_service.path("");
    let session = AccountSession {
        id: String::from("comcast"),
        token: String::from("token_id"),
        account_id: String::from("6753750"),
        device_id: String::from("4523652"),
    };
    let sessions_params = SessionParams {
        app_id: "refui".to_string(),
        dist_session: session,
    };
    let content_access_availabilities = vec![ContentAccessAvailability {
        _type: "channel-lineup-contract".to_string(),
        id: "partner.com/availability/123".to_string(),
        end_time: Some(String::from("2021-04-23T18:25:43.511Z")),
        start_time: Some(String::from("2021-04-23T18:25:43.511Z")),
        catalog_id: Some(String::from("catalog_id")),
    }];
    let content_access_entitlements = vec![ContentAccessEntitlement {
        end_time: Some(String::from("2021-04-23T18:25:43.511Z")),
        start_time: Some(String::from("2021-04-23T18:25:43.511Z")),
        entitlement_id: "test-entitlement-firecert-contract".to_string(),
    }];
    let content_access_params = ContentAccessInfo {
        availabilities: Some(content_access_availabilities),
        entitlements: Some(content_access_entitlements),
    };
    let params = ContentAccessListSetParams {
        session_info: sessions_params,
        content_access_info: content_access_params,
    };

    let result = XvpSession::set_content_access(xvp_url.to_string(), params).await;
    assert!(result.is_ok());
}

#[tokio::test(flavor = "multi_thread")]
#[cfg_attr(not(feature = "http_contract_tests"), ignore)]
async fn test_set_content_access_contract_with_availabilities() {
    let xvp_service = PactBuilder::new("ripple", "xvp-service")
    .interaction("a request to set content access", "", |mut i| {
        i.given("content access service has been up and running");
        i.request.method("PUT");
        i.request.path(term!("^/partners/comcast/accounts/[0-9]*/appSettings/[a-zA-Z0-9]*$", "/partners/comcast/accounts/1234512345/appSettings/refui"));
        i.request.content_type("application/json");
        i.request.header("Authorization", term!("^Bearer [a-zA-Z0-9_.=]*$", "Bearer eyJraWQiOiJzYXQtcHJvZC1rMS0xMDI0IiwiYWxnIjoiUlMyNTYifQ.eyJwcmluY_lwYWwiOnsiZGdkIjpbTVEsMm0IMoXbF2mLrJZ6DmEQf7_YAe9U=="));
        i.request.json_body(json_pattern!(
            {
                "availabilities": [
                    {
                        "type": like!("channel-lineup"),
                        "id": like!("partner.com/availability/123"),
                        "startTime": datetime!("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'","2021-04-23T18:25:43.511Z"),
                        "endTime": datetime!("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'","2021-04-23T18:25:43.511Z"),
                        "catalogId": like!("catalog_id1")
                    },
                    {
                        "type": like!("channel-lineup"),
                        "id": like!("partner.com/availability/456"),
                        "startTime": datetime!("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'","2021-04-23T18:25:43.511Z"),
                        "endTime": datetime!("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'","2021-04-23T18:25:43.511Z"),
                        "catalogId": like!("catalog_id2")
                    }
                ]
            }
        ));
        i.request.query_param("deviceId", term!("^[0-9]*$", "123"));
        i.request.query_param("clientId", term!("^[0-9a-zA-Z]*$", "ripple"));
        i.response
            .content_type("application/json")
            .status(200);
        i}).start_mock_server(None);

    let xvp_url = xvp_service.path("");
    let session = AccountSession {
        id: String::from("comcast"),
        token: String::from("token_id"),
        account_id: String::from("6753750"),
        device_id: String::from("4523652"),
    };
    let sessions_params = SessionParams {
        app_id: "refui".to_string(),
        dist_session: session,
    };
    let content_access_availabilities = vec![
        ContentAccessAvailability {
            _type: "channel-lineup-contract".to_string(),
            id: "partner.com/availability/123Contract".to_string(),
            end_time: Some(String::from("2021-04-23T18:25:43.511Z")),
            start_time: Some(String::from("2021-04-23T18:25:43.511Z")),
            catalog_id: Some(String::from("catalog_id")),
        },
        ContentAccessAvailability {
            _type: "channel-lineup-contract".to_string(),
            id: "partner.com/availability/456Contract".to_string(),
            end_time: Some(String::from("2021-04-23T18:25:43.511Z")),
            start_time: Some(String::from("2021-04-23T18:25:43.511Z")),
            catalog_id: Some(String::from("catalog_id")),
        },
    ];
    let content_access_entitlements: Option<Vec<ContentAccessEntitlement>> = None;

    let content_access_params = ContentAccessInfo {
        availabilities: Some(content_access_availabilities),
        entitlements: content_access_entitlements,
    };
    let params = ContentAccessListSetParams {
        session_info: sessions_params,
        content_access_info: content_access_params,
    };

    let result = XvpSession::set_content_access(xvp_url.to_string(), params).await;
    assert!(result.is_ok());
}

#[tokio::test(flavor = "multi_thread")]
#[cfg_attr(not(feature = "http_contract_tests"), ignore)]
async fn test_clear_content_access_contract() {
    let xvp_service = PactBuilder::new("ripple", "xvp-service")
    .interaction("a request to clear content access with availabilities", "", |mut i| {
        i.given("content access service has been up and running");
        i.request.method("PUT");
        i.request.path(term!("^/partners/comcast/accounts/[0-9]*/appSettings/[a-zA-Z0-9]*$", "/partners/comcast/accounts/1234512345/appSettings/refui"));
        i.request.content_type("application/json");
        i.request.header("Authorization", term!("^Bearer [a-zA-Z0-9_.=]*$", "Bearer eyJraWQiOiJzYXQtcHJvZC1rMS0xMDI0IiwiYWxnIjoiUlMyNTYifQ.eyJwcmluY_lwYWwiOnsiZGdkIjpbTVEsMm0IMoXbF2mLrJZ6DmEQf7_YAe9U=="));
        i.request.json_body(json_pattern!(
            {
                "availabilities": [],
                "entitlements": []
            }
        ));
        i.request.query_param("deviceId", term!("^[0-9]*$", "123"));
        i.request.query_param("clientId", term!("^[0-9a-zA-Z]*$", "ripple"));
        i.response
            .content_type("application/json")
            .status(200);
        i}).start_mock_server(None);

    let xvp_url = xvp_service.path("");
    let session = AccountSession {
        id: String::from("comcast"),
        token: String::from("token_id"),
        account_id: String::from("6753750"),
        device_id: String::from("4523652"),
    };
    let sessions_params = SessionParams {
        app_id: "refui".to_string(),
        dist_session: session,
    };

    let params = ClearContentSetParams {
        session_info: sessions_params,
    };

    let result = XvpSession::clear_content_access(xvp_url.to_string(), params).await;
    assert!(result.is_ok());
}
