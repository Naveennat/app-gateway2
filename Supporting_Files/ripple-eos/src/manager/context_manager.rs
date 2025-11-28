use std::str::FromStr;

use crate::state::distributor_state::DistributorState;
use thunder_ripple_sdk::client::device_operator::{
    DeviceOperator, DeviceResponseMessage, DeviceSubscribeRequest,
};
use thunder_ripple_sdk::client::thunder_plugin::ThunderPlugin;
use thunder_ripple_sdk::processors::thunder_device_info::{
    ThunderDeviceInfoRequestProcessor, ThunderNetworkService,
};
use thunder_ripple_sdk::ripple_sdk::api::context::RippleContextUpdateRequest;
use thunder_ripple_sdk::ripple_sdk::api::device::device_request::PowerState;
use thunder_ripple_sdk::ripple_sdk::api::device::device_request::SystemPowerState;
use thunder_ripple_sdk::ripple_sdk::log::{error, info};
use thunder_ripple_sdk::ripple_sdk::tokio;
use tokio::sync::mpsc;

pub struct ContextManager;

impl ContextManager {
    pub fn setup(ps_c: DistributorState) {
        let thunder = ps_c.get_thunder();
        // Asynchronously get context and update the state
        tokio::spawn(async move {
            // Setup listeners here
            // Check Internet
            let _ = ThunderNetworkService::has_internet(&thunder);

            // Get Initial power state
            if let Ok(v) =
                ThunderDeviceInfoRequestProcessor::get_power_state(&ps_c.get_thunder()).await
            {
                ps_c.get_client()
                    .context_update(RippleContextUpdateRequest::PowerState(SystemPowerState {
                        current_power_state: v.clone(),
                        power_state: v,
                    }));
            }

            let request = DeviceSubscribeRequest {
                module: ThunderPlugin::System.callsign_and_version(),
                event_name: "onSystemPowerStateChanged".to_owned(),
                params: None,
                sub_id: None,
            };
            let (sub_tx, mut sub_rx) = mpsc::channel::<DeviceResponseMessage>(5);
            let client = ps_c.thunder.get_thunder_client();

            if let Err(e) = client.subscribe(request, sub_tx).await {
                error!("Failed to subscribe to powerstate : {:?}", e);
            } else {
                info!("Successfully subscribed to powerstate");
            }
            let extn_client = ps_c.thunder.get_client();
            while let Some(message) = sub_rx.recv().await {
                if let Some(power_state) = message.message.get("powerState") {
                    if let Some(current_power_state) = message.message.get("currentPowerState") {
                        if let Ok(v) = serde_json::to_string(power_state) {
                            if let Ok(c) = serde_json::to_string(current_power_state) {
                                let _ = extn_client.request_transient(
                                    RippleContextUpdateRequest::PowerState(SystemPowerState {
                                        current_power_state: PowerState::from_str(c.as_str())
                                            .unwrap(),
                                        power_state: PowerState::from_str(v.as_str()).unwrap(),
                                    }),
                                );
                            }
                        }
                    }
                }
            }
        });
    }
}
