use crate::tests::contracts::utils::get_crate_source_path;
use ottx_protos::ad_platform::{
    ad_platform_service_client::AdPlatformServiceClient, AdRouterRequest,
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
        "{}/target/source-ottx-protobuf/src/main/resources/ad_platform.proto",
        crate_source_path,
    );

    proto_file_path
}

pub async fn test_ad_platform() {
    test_get_ad_config().await;
}

async fn test_get_ad_config() {
    let mut pact_builder = PactBuilderAsync::new_v4("ripple", "ad-platform-service");

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
            "pact:proto-service": "AdPlatformService/GetAdRouter",
            "request": {
                "durable_app_id": "matching(type, 'test_app')",
                "environment": "matching(type, 'test_environment')",
            },
            "response": {
                "ad_server_url": "matching(type, 'ad_server_url')",
                "ad_server_url_template": "matching(type, 'ad_server_url_template')",
                "ad_network_i_d": "matching(type, 'ad_network_id')",
                "ad_profile_i_d": "matching(type, 'ad_profile_id')",
                "ad_site_section_i_d": "matching(type, 'ad_site_section_id')",
            }
            }))
            .await;
            i
        })
        .await
        .start_mock_server_async(Some("protobuf/transport/grpc"))
        .await;

    let ad_platform_url = mock_server.url();
    let mut client = AdPlatformServiceClient::connect(ad_platform_url.to_string())
        .await
        .unwrap_or_else(|err| panic!("Failed to connect to the gRPC mock server: {}", err));

    let perm_req = tonic::Request::new(AdRouterRequest {
        durable_app_id: "test_app".to_string(),
        environment: "test_environment".to_string(),
    });

    let result = client.get_ad_router(perm_req).await;

    assert!(
        result.is_ok(),
        "Failed to get ad config: {:?}",
        result.err()
    );

    let response = result.unwrap().into_inner();
    assert_eq!(response.ad_server_url, "ad_server_url");
    assert_eq!(response.ad_server_url_template, "ad_server_url_template");
    assert_eq!(response.ad_network_i_d, "ad_network_id");
    assert_eq!(response.ad_profile_i_d, "ad_profile_id");
    assert_eq!(response.ad_site_section_i_d, "ad_site_section_id");
}
