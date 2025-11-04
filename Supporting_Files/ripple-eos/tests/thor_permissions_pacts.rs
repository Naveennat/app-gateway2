use crate::tests::contracts::utils::get_crate_source_path;
use ottx_protos::permission_service::{
    app_permissions_service_client::AppPermissionsServiceClient, AppKey,
    EnumeratePermissionsRequest, GetThorTokenRequest,
};
use pact_consumer::mock_server::StartMockServerAsync;
use pact_consumer::prelude::*;
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
        "{}/target/source-ottx-protobuf/src/main/resources/permission_service.proto",
        crate_source_path,
    );

    proto_file_path
}

pub async fn test_permission_service() {
    test_get_thor_token().await;
    test_get_app_permissions().await;
}

async fn test_get_thor_token() {
    let mut pact_builder = PactBuilderAsync::new_v4("ripple", "permission_service");

    let proto_file = get_proto_file_path();

    let mock_server = pact_builder
        .using_plugin("protobuf", None)
        .await
        .synchronous_message_interaction("get config", |mut i| async move {
            let proto = Path::new(&proto_file)
                .canonicalize()
                .unwrap()
                .to_string_lossy()
                .to_string();
            i.contents_from(json!({
            "pact:proto": proto,
            "pact:content-type": "application/protobuf",
            "pact:proto-service": "AppPermissionsService/GetThorToken",
            "request": {
                "app": "matching(type, 'test_app_id')",
                "content_provider": "matching(type, 'test_content_provider')",
                "device_session_id": "matching(type, 'test_device_session_id')",
                "app_session_id": "matching(type, 'test_app_session_id')",
                "token_mode": "matching(type, 'untrusted')",
                "ttl": "matching(type, 3600)"
            },
            "response": {
                "token": "matching(type, 'thor_token')"
            }
            }))
            .await;
            i
        })
        .await
        .start_mock_server_async(Some("protobuf/transport/grpc"))
        .await;
    let permission_service_url = mock_server.url();
    let mut client = AppPermissionsServiceClient::connect(permission_service_url.to_string())
        .await
        .unwrap_or_else(|err| panic!("Failed to connect to the gRPC mock server: {}", err));

    let request = tonic::Request::new(GetThorTokenRequest {
        app: "test_app_id".to_string(),
        content_provider: "test_content_provider".to_string(),
        device_session_id: "test_device_session_id".to_string(),
        app_session_id: "test_app_session_id".to_string(),
        token_mode: "untrusted".to_string(),
        ttl: 3600,
    });

    let result = client.get_thor_token(request).await;

    assert!(
        result.is_ok(),
        "Failed to get Thor token: {:?}",
        result.err()
    );

    let response = result.unwrap().into_inner();
    assert_eq!(response.token, "thor_token");
}

async fn test_get_app_permissions() {
    let mut pact_builder = PactBuilderAsync::new_v4("ripple", "permission_service");

    let proto_file = get_proto_file_path();

    let mock_server = pact_builder
        .using_plugin("protobuf", None)
        .await
        .synchronous_message_interaction("enumerate permissions", |mut i| async move {
            let proto = Path::new(&proto_file)
                .canonicalize()
                .unwrap()
                .to_string_lossy()
                .to_string();
            i.contents_from(json!({
                "pact:proto": proto,
                "pact:content-type": "application/protobuf",
                "pact:proto-service": "AppPermissionsService/EnumeratePermissions",
                "request": {
                    "app_key": {
                        "app": "matching(type, 'test_app_id')",
                        "syndication_partner_id": "matching(type, 'test_partner_id')"
                    },
                    "permission_filters": "matching(type, 'permission_filter')"
                },
                "response": {
                    "permissions": ["matching(type, 'permission_1')", "matching(type, 'permission_2')"]
                }
            }))
            .await;
            i
        })
        .await
        .start_mock_server_async(Some("protobuf/transport/grpc"))
        .await;

    let permission_service_url = mock_server.url();
    let mut client = AppPermissionsServiceClient::connect(permission_service_url.to_string())
        .await
        .unwrap_or_else(|err| panic!("Failed to connect to the gRPC mock server: {}", err));

    let request = tonic::Request::new(EnumeratePermissionsRequest {
        app_key: Some(AppKey {
            app: "test_app_id".to_string(),
            syndication_partner_id: "test_partner_id".to_string(),
        }),
        permission_filters: vec!["permission_filter".to_string()],
    });

    let result = client.enumerate_permissions(request).await;

    assert!(
        result.is_ok(),
        "Failed to enumerate permissions: {:?}",
        result.err()
    );

    let response = result.unwrap().into_inner();
    assert_eq!(response.permissions, vec!["permission_1", "permission_2"]);
}
