pub mod gateway {
    pub mod appsanity_gateway;
}

pub mod service {
    pub mod appsanity_account_link;
    pub mod appsanity_advertising;
    pub mod appsanity_auth;
    pub mod appsanity_discovery;
    pub mod appsanity_metrics;
    pub mod appsanity_privacy;
    pub mod ott_token;
    pub mod thor_permission;
    pub mod xvp_sync_and_monitor;
}

pub mod util {
    pub mod channel_util;
    pub mod cloud_linchpin_monitor;
    pub mod cloud_periodic_sync;
    pub mod cloud_sync_monitor_utils;
    pub mod http_client;
    pub mod service_util;
    pub mod sync_settings;
}
pub mod manager {
    pub mod context_manager;
    pub mod data_governor;
    pub mod privacy_response_manager;
    pub mod session_manager;
}
pub mod client {
    pub mod xvp_playback;
    pub mod xvp_session;
    pub mod xvp_videoservice;
    pub mod xvp_xifaservice;
}

pub mod sync_and_monitor {
    pub mod privacy_sync_monitor;
    pub mod user_grants_sync_monitor;
}

pub mod extn {
    pub mod appsanity_permission_processor;
    pub mod appsanity_privacy_processor;
    pub mod appsanity_usergrants_processor;
    pub mod crypto_util;
    pub mod eos_ffi;
    pub mod extn_utils;
    pub mod thunder {
        pub mod events {
            pub mod comcast_thunder_event_handlers;
        }
        pub mod thunder_session;
    }
}

pub mod message;
pub mod state {
    pub mod distributor_state;
    pub mod sift_metrics_state;
}

pub mod model {
    pub mod permissions {
        pub mod api_grants;
        pub mod firebolt;
        pub mod permissions;
        pub mod thor_permission_registry;
    }
    pub mod auth;
    pub mod badger;
    pub mod metrics;
}

pub mod rpc {
    pub mod eos_account_rpc;
    pub mod eos_advertising_rpc;
    pub mod eos_authentication_rpc;
    pub mod eos_device_rpc;
    pub mod eos_discovery_rpc;
    pub mod eos_metrics_management_rpc;
    pub mod eos_metrics_rpc;
    pub mod eos_privacy_rpc;
    pub mod rpc_utils;
    pub mod setup_distributor_rpc_handlers;
}

extern crate tokio;

#[macro_export]
macro_rules! thunder_pubsub_state {
    ($server_url: ident) => {{
        ThunderPubSubState::new(ripple_eos_test_utils::thunder_state!($server_url))
    }};
}

#[cfg(test)]
pub mod tests {
    #[cfg(any(
        feature = "contract_tests",
        feature = "http_contract_tests",
        feature = "websocket_contract_tests",
        feature = "grpc_contract_tests"
    ))]
    pub mod contracts {
        pub mod appsanity_ad_platform_pacts;
        pub mod appsanity_discovery_pacts;
        pub mod appsanity_grpc_pacts;
        pub mod appsanity_playback_pacts;

        pub mod appsanity_video_services_pacts;
        pub mod appsanity_xvp_xifa_pacts;
        pub mod ott_token_service_pacts;
        pub mod thor_permissions_pacts;
        pub mod utils;
    }
}

use crate::extn::eos_ffi;

#[tokio::main(worker_threads = 2)]
async fn main() {
    eos_ffi::start_service().await;
}
