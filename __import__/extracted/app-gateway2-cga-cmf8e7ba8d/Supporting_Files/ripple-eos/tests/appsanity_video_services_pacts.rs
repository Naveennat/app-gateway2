use crate::client::xvp_videoservice::XvpVideoService;
use pact_consumer::prelude::*;
use pact_consumer::*;
use ripple_sdk::api::{
    firebolt::fb_discovery::{SessionParams, SignInRequestParams},
    session::AccountSession,
};

#[tokio::test(flavor = "multi_thread")]
#[cfg_attr(not(feature = "http_contract_tests"), ignore)]
async fn test_video_service_sign_in_contract() {
    let xvp_service = PactBuilder::new("ripple", "xvp-service")
    .interaction("When the user is Authorized in the system", "", |mut interaction_builder| {
        interaction_builder.given("When the user tries to signin");
        interaction_builder.request.method("PUT");
        interaction_builder.request.path(term!("/partners/[a-zA-Z0-9]*/accounts/[0-9]*/videoServices/xrn:xvp:application:[a-zA-Z_]*/engaged", "/partners/comcast/accounts/1234512345/videoServices/xrn:xvp:application:refui/engaged"));
        interaction_builder.request.content_type("application/json;  charset=UTF-8");
        interaction_builder.request.header("authorization", term!("^Bearer [a-zA-Z0-9_.=]*$", "Bearer eyJraWQiOiJzYXQtcHJvZC1rMS0xMDI0IiwiYWxnIjoiUlMyNTYifQ.eyJwcmluY_lwYWwiOnsiZGdkIjpbTVEsMm0IMoXbF2mLrJZ6DmEQf7_YAe9U=="));
        interaction_builder.request.json_body(json_pattern!(
            {
                "eventType": like!("signIn"),
                "isSignedIn": like!(true)
            }
        ));
        interaction_builder.request.query_param("ownerReference", term!("xrn:xcal:subscriber:account:[0-9.]*", "xrn:xcal:subscriber:account:123"));
        interaction_builder.request.query_param("clientId", term!("^[0-9a-zA-Z]*$", "ripple"));
        interaction_builder.response
            .content_type("application/json")
            .status(200)
            .json_body(json_pattern!(
                {
                    "partnerId": like!("xGlobal"),
                    "accountId": like!("6213445014373683020"),
                    "ownerReference": like!("xrn:subscriber:account:6213445014373683020"),
                    "entityUrn": like!("xrn:xvp:application:Comcast_StreamApp"),
                    "entityId": like!("Comcast_StreamApp"),
                    "entityType": like!("Application"),
                    "durableAppId": like!("Comcast_StreamApp"),
                    "eventType": like!("signIn"),
                    "added": datetime!("yyyy-MM-dd'T'HH:mm:ss'Z'","2024-08-17T12:00:00Z"),
                    "updated": datetime!("yyyy-MM-dd'T'HH:mm:ss'Z'","2024-08-17T12:00:00Z")
                  }
            ));
        interaction_builder}).start_mock_server(None);

    let xvp_url = xvp_service.path("");
    let distributer_session = AccountSession {
        id: String::from("comcast"),
        token: String::from("eyJraWQiOiJzYXQtcHJvZC1rMS0xMDI0IiwiYWxnIjoiUlMyNTYifQ.eyJwcmluY_lwYWwiOnsiZGdkIjpbTVEsMm0IMoXbF2mLrJZ6DmEQf7_YAe9U=="),
        account_id: String::from("6753750"),
        device_id: String::from("4523652"),
    };
    let sessions_params = SessionParams {
        app_id: "refui".to_string(),
        dist_session: distributer_session,
    };

    let params = SignInRequestParams {
        session_info: sessions_params,
        is_signed_in: true,
    };

    let result = XvpVideoService::sign_in(xvp_url.to_string().as_str(), "account", params).await;
    assert!(result.is_ok());
}

#[tokio::test(flavor = "multi_thread")]
#[cfg_attr(not(feature = "http_contract_tests"), ignore)]
async fn test_video_service_unauth_sign_in_contract() {
    let xvp_service = PactBuilder::new("ripple", "xvp-service")
    .interaction("When the user is NOT Authorized in the system", "", |mut interaction_builder| {
        interaction_builder.given("When the user tries to signin");
        interaction_builder.request.method("PUT");
        interaction_builder.request.path(term!("/partners/[a-zA-Z0-9]*/accounts/[0-9]*/videoServices/xrn:xvp:application:[a-zA-Z_]*/engaged", "/partners/comcast/accounts/1234512345/videoServices/xrn:xvp:application:refui/engaged"));
        interaction_builder.request.content_type("application/json;  charset=UTF-8");
        interaction_builder.request.header("authorization", term!("^Bearer [a-zA-Z0-9_.=]*$", "Bearer eyJraWQiOiJzYXQtcHJvZC1rMS0xMDI0IiwiYWxnIjoiUlMyNTYifQ.eyJwcmluY_lwYWwiOnsiZGdkIjpbTVEsMm0IMoXbF2mLrJZ6DmEQf7_YAe9U=="));
        interaction_builder.request.json_body(json_pattern!(
            {
                "eventType": like!("signIn"),
                "isSignedIn": like!(true)
            }
        ));
        interaction_builder.request.query_param("ownerReference", term!("xrn:xcal:subscriber:account:[0-9.]*", "xrn:xcal:subscriber:account:123"));
        interaction_builder.request.query_param("clientId", term!("^[0-9a-zA-Z]*$", "ripple"));
        interaction_builder.response
            .content_type("application/json")
            .status(403);
        interaction_builder}).start_mock_server(None);

    let xvp_url = xvp_service.path("");
    let distributer_session = AccountSession {
        id: String::from("comcast"),
        token: String::from("eyJraWQiOiJzYXQtcHJvZC1rMS0xMDI0IiwiYWxnIjoiUlMyNTYifQ.eyJwcmluY_lwYWwiOnsiZGdkIjpbTVEsMm0IMoXbF2mLrJZ6DmEQf7_YAe9U=="),
        account_id: String::from("6753750"),
        device_id: String::from("4523652"),
    };
    let sessions_params = SessionParams {
        app_id: "refui".to_string(),
        dist_session: distributer_session,
    };

    let params = SignInRequestParams {
        session_info: sessions_params,
        is_signed_in: true,
    };

    let result = XvpVideoService::sign_in(xvp_url.to_string().as_str(), "account", params).await;
    assert!(result.is_err());
}
