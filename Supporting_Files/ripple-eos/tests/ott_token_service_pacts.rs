use crate::tests::contracts::utils::get_crate_source_path;
use ottx_protos::ott_token::{
    ott_token_service_client::OttTokenServiceClient, PlatformTokenRequest,
};
use pact_consumer::mock_server::StartMockServerAsync;
use pact_consumer::prelude::*;
use ripple_sdk::api::session::AccountSession;
use serde_json::json;
use std::path::Path;

fn get_proto_file_path() -> String {
    let crate_source_path =
        match get_crate_source_path("partners.artifactory.comcast.com", "ottx_protos") {
            Some(path) => path,
            None => {
                assert!(false);
                return "".into();
            }
        };

    let proto_file_path = format!(
        "{}/target/source-ottx-protobuf/src/main/resources/ott_token.proto",
        crate_source_path,
    );

    proto_file_path
}

pub async fn test_ott_token_service() {
    test_get_ott_token().await;
}

async fn test_get_ott_token() {
    let mut pact_builder = PactBuilderAsync::new_v4("ripple", "ott-token-service");

    let proto_file = get_proto_file_path();

    let mock_server = pact_builder
        .using_plugin("protobuf", None)
        .await
        .synchronous_message_interaction("get ott token", |mut i| async move {
            let proto = Path::new(&proto_file)
                .canonicalize()
                .unwrap()
                .to_string_lossy()
                .to_string();
            i.contents_from(json!({
                "pact:proto": proto,
                "pact:content-type": "application/protobuf",
                "pact:proto-service": "OttTokenService/PlatformToken",
                "request": {
                    "xact": "matching(type, 'test_xact')",
                    "sat": "matching(type, 'test_sat')",
                    "app_id": "matching(type, 'test_app_id')"
                },
                "response": {
                    "platform_token": "matching(type, 'platform_token')",
                    "token_type": "matching(type, 'Bearer')",
                    "scope": "matching(type, 'test_scope')",
                    "expires_in": "matching(type, 3600)"
                }
            }))
            .await;
            i
        })
        .await
        .start_mock_server_async(Some("protobuf/transport/grpc"))
        .await;

    let ott_token_url = mock_server.url();
    let mut client = OttTokenServiceClient::connect(ott_token_url.to_string())
        .await
        .unwrap_or_else(|err| panic!("Failed to connect to the gRPC mock server: {}", err));

    let account_session = AccountSession {
        token: "test_sat".to_string(),
        ..Default::default()
    };
    let app_id = "test_app_id".to_string();
    let xact = "test_xact".to_string();

    let request = tonic::Request::new(PlatformTokenRequest {
        xact: xact.clone(),
        sat: account_session.token.clone(),
        app_id: app_id.clone(),
    });

    let result = client.platform_token(request).await;

    assert!(
        result.is_ok(),
        "Failed to get OTT token: {:?}",
        result.err()
    );

    let response = result.unwrap().into_inner();
    assert_eq!(response.platform_token, "platform_token");
    assert_eq!(response.token_type, "Bearer");
    assert_eq!(response.scope, "test_scope");
    assert_eq!(response.expires_in, 3600);
}
