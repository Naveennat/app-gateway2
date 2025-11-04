use crate::tests::contracts::appsanity_ad_platform_pacts::test_ad_platform;
use crate::tests::contracts::ott_token_service_pacts::test_ott_token_service;
use crate::tests::contracts::thor_permissions_pacts::test_permission_service;

#[tokio::test(flavor = "multi_thread")]
#[cfg_attr(not(feature = "grpc_contract_tests"), ignore)]
async fn test_grpc() {
    test_ad_platform().await;
    test_ott_token_service().await;
    test_permission_service().await;
}
