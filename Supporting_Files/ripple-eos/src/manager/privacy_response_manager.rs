use ripple_sdk::api::gateway::rpc_gateway_api::RpcRequest;
use thunder_ripple_sdk::ripple_sdk::api::distributor::distributor_privacy::{
    PrivacyResponse, PrivacySettingsData,
};
use thunder_ripple_sdk::ripple_sdk::log::debug;
use thunder_ripple_sdk::ripple_sdk::tokio::sync::mpsc;
use tokio::sync::mpsc::Sender;

use crate::state::distributor_state::DistributorState;

pub struct PrivacyResponseManager;

use tokio::sync::mpsc::Receiver;

impl PrivacyResponseManager {
    pub async fn process_cloud_updates(state: DistributorState, mut rx: Receiver<PrivacyResponse>) {
        let client = state.get_client();
        tokio::spawn(async move {
            debug!("Starting cloud update processor");
            let mut syn_grant_map_with_policy = true;
            while let Some(dpab_response_payload) = rx.recv().await {
                debug!("Received cloud update payload: {:?}", dpab_response_payload);
                match dpab_response_payload {
                    PrivacyResponse::Settings(privacy_settings) => {
                        state.update_privacy(privacy_settings.clone());
                        let privacy_settings_data: PrivacySettingsData = privacy_settings.into();
                        debug!("Privacy settings data: {:?}", privacy_settings_data);
                        let privacydata_req = RpcRequest::get_new_internal(
                            "ripple.setPrivacySettings".into(),
                            Some(serde_json::to_value(privacy_settings_data.clone()).unwrap()),
                        );
                        let send_res = client.clone().request(privacydata_req).await;
                        debug!("update send result: {:?}", send_res);
                    }
                    PrivacyResponse::Grants(cloud_user_grants) => {
                        // There is no way of knowing deleted entries when device/ripple was offline
                        // Delete all entries matching persistence == PolicyPersistenceType::Account

                        let clear_grants_req =
                            RpcRequest::get_new_internal("ripple.clearUserGrants".into(), None);
                        let send_res = client.clone().request(clear_grants_req).await;
                        debug!("clear all user grants send result: {:?}", send_res);

                        // Valid entries from cloud will be re-inserted below
                        for entry in cloud_user_grants {
                            let set_grants_params = serde_json::to_value(&entry).ok();
                            let set_grants_req = RpcRequest::get_new_internal(
                                "ripple.setUserGrants".into(),
                                set_grants_params,
                            );
                            let send_res = client.clone().request(set_grants_req).await;
                            debug!("update usergrants send result: {:?}", send_res);
                        }

                        if syn_grant_map_with_policy {
                            // Check user grant capabilities exist in grant policy during boot up.
                            // If the policy no longer exists then remove whatever is in local storage, grant state and cloud storage
                            syn_grant_map_with_policy = false;
                            let sync_grant_map_req =
                                RpcRequest::get_new_internal("ripple.syncGrantsMap".into(), None);
                            let send_res = client.clone().request(sync_grant_map_req).await;
                            debug!(
                                "sync user grant map with policy send result: {:?}",
                                send_res
                            );
                        }
                    }
                    _ => {
                        debug!("Unknown response received");
                    }
                }
            }
        });
    }

    pub fn start(state: DistributorState) -> Sender<PrivacyResponse> {
        let (response_tx, response_rx) = mpsc::channel::<PrivacyResponse>(32);
        let state_c = state.clone();
        let _ = tokio::spawn(async move {
            Self::process_cloud_updates(state_c, response_rx).await;
        });
        response_tx
    }
}
