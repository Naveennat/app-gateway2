use jsonrpsee::core::server::rpc_module::Methods;
use ripple_sdk::api::manifest::device_manifest::DeviceManifest;
use ripple_sdk::api::manifest::extn_manifest::{ExtnManifest, ExtnSymbol};
use ripple_sdk::api::manifest::ripple_manifest_loader::RippleManifestLoader;
use ripple_sdk::extn::ffi::ffi_channel::ExtnChannel;
use ripple_sdk::serde_json;
use ripple_sdk::service::service_client::ServiceClient;
use ripple_sdk::service::service_message::JsonRpcNotification;
use ripple_sdk::{
    export_extn_channel,
    extn::extn_id::{ExtnClassId, ExtnId},
    log::{debug, error, info},
    service::service_message::{JsonRpcMessage, ServiceMessage},
    tokio,
    utils::{extn_utils::ExtnUtils, logger::init_and_configure_logger},
};
use serde_json::Value;
use std::collections::HashMap;
use std::str;
use thunder_ripple_sdk::bootstrap::boot_thunder::boot_thunder;
use thunder_ripple_sdk::ripple_sdk::api::context::{ActivationStatus, RippleContextUpdateType};
use tokio::sync::mpsc;

use crate::{
    badger_state::BadgerState,
    handlers::{
        badger_device_info::subscribe_to_device_changes,
        badger_info_rpc::{get_account_session, BadgerServiceImpl, BadgerServiceServer},
    },
    thunder::thunder_session::ThunderSessionService,
};
use ripple_sdk::api::context::RippleContext;
include!(concat!(env!("OUT_DIR"), "/version.rs"));

const EXTN_NAME: &str = "badger";
pub const CHANNEL_NAME: &str = "badger";
pub const BADGER_SERVICE_ID: &str = "ripple:channel:gateway:badger";

fn get_rpc_methods_for_service(state: &BadgerState) -> Methods {
    let mut methods = Methods::new();
    let _ = methods.merge(BadgerServiceImpl::new(&state).into_rpc());
    methods
}

pub async fn start_service() {
    let log_lev = ripple_sdk::log::LevelFilter::Debug;
    let _ = init_and_configure_logger(
        SEMVER_LIGHTWEIGHT,
        EXTN_NAME.into(),
        Some(vec![
            ("extn_manifest".to_string(), log_lev),
            ("device_manifest".to_string(), log_lev),
        ]),
    );
    info!("Starting badger channel");
    let Ok((extn_manifest, device_manifest)) = RippleManifestLoader::initialize() else {
        error!("Error initializing manifests");
        return;
    };

    let symbol = get_symbol(extn_manifest);
    let service_client = if let Some(symbol) = symbol {
        ServiceClient::builder().with_extension(symbol).build()
    } else {
        ServiceClient::builder().build()
    };
    init(service_client.clone(), device_manifest).await;
}

fn start() {
    info!("Starting badger channel");
    let Ok((extn_manifest, device_manifest)) = RippleManifestLoader::initialize() else {
        error!("Error initializing manifests");
        return;
    };
    let symbol = get_symbol(extn_manifest);
    let service_client = if let Some(symbol) = symbol {
        ServiceClient::builder().with_extension(symbol).build()
    } else {
        ServiceClient::builder().build()
    };
    let runtime = ExtnUtils::get_runtime("e-b".to_owned(), service_client.get_stack_size());

    runtime.block_on(async move {
        init(service_client.clone(), device_manifest).await;
    });
}

fn get_symbol(extn_manifest: ExtnManifest) -> Option<ExtnSymbol> {
    let extn = ExtnId::new_channel(ExtnClassId::Gateway, EXTN_NAME.to_string()).to_string();
    extn_manifest.get_extn_symbol(&extn)
}

pub async fn init(client: ServiceClient, device_manifest: DeviceManifest) {
    // Return error if service_client is not holding an extn client internally.
    // This will be fixed when we remove extension client completely.
    if let Some(extn_client) = client.get_extn_client() {
        // extn_client is available, proceed as before
        let client_for_thunder = extn_client.clone();

        let mut client_c_for_spawn = client.clone();
        let client_c_for_init = client.clone();

        let device_manifest_c = device_manifest.clone();
        ripple_sdk::tokio::spawn(async move {
            info!("Booting thunder");
            let boot_result = tokio::time::timeout(tokio::time::Duration::from_secs(5000), async {
                boot_thunder(client_for_thunder, &device_manifest_c).await
            })
            .await;

            match boot_result {
                Ok(Some(state)) => {
                    let cached_state =
                        BadgerState::new(state.state.clone(), device_manifest.clone(), client);

                    let _ = client_c_for_spawn
                        .clone()
                        .set_service_rpc_route(get_rpc_methods_for_service(&cached_state));

                    // Get account session during badger boot up.
                    // This is required in case of badger misses token update event notification during boot up.
                    // This could be happened when badger subscibes to token changed event after token event notification is sent
                    info!("Get account session during badger boot up");
                    let cached_state_clone = cached_state.clone();
                    tokio::spawn(async move {
                        get_account_session(&cached_state_clone).await;
                    });

                    info!("Initializing device event subscriptions");
                    setup_token_event_handler(&cached_state, &mut client_c_for_spawn);

                    subscribe_to_device_changes(&cached_state.clone()).await;
                }
                Ok(None) | Err(_) => {
                    error!("Failed to boot thunder");
                }
            }
        });
        client_c_for_init.initialize().await;
    } else {
        error!("Service client does not hold an extn client. Cannot start badger extension.");
        return;
    }
}

fn setup_token_event_handler(state: &BadgerState, service_client: &mut ServiceClient) {
    let (tx, mut rx) = mpsc::channel::<ServiceMessage>(10);
    let mut service_client = service_client.clone();
    let state = state.clone();

    tokio::spawn(async move {
        let result = service_client
            .call_and_parse_ripple_event_subscription_req_rpc(
                "ripple.onContextTokenChangedEvent",
                None,
                None,
                5000,
                BADGER_SERVICE_ID,
                "Failed to request token changed notification",
                tx,
            )
            .await;

        match result {
            Ok(_) => {
                while let Some(sm) = rx.recv().await {
                    debug!("[REFRESH TOKEN] received context event {:?}", sm);
                    handle_service_message(&state, sm).await;
                }
            }
            Err(e) => {
                error!("Error sending service request: {:?}", e);
            }
        }
    });
}

async fn handle_service_message(state: &BadgerState, sm: ServiceMessage) {
    match sm.message {
        JsonRpcMessage::Notification(ref notification) => {
            match parse_ripple_context(notification) {
                Some(ripple_context) => {
                    handle_ripple_context(state, ripple_context).await;
                }
                None => {
                    error!(
                        "Failed to parse RippleContext from notification: missing or invalid data"
                    );
                }
            }
        }
        _ => {
            error!("Received unexpected JsonRpc message");
        }
    }
}

fn parse_ripple_context(notification: &JsonRpcNotification) -> Option<RippleContext> {
    let params = notification.params.clone().unwrap_or_default();
    let params_map: HashMap<String, Value> = match serde_json::from_value(params) {
        Ok(map) => map,
        Err(e) => {
            error!("Failed to parse params as HashMap: {}", e);
            return None;
        }
    };
    let ripple_context_value = match params_map.get("ripple_context") {
        Some(val) => val.clone(),
        None => {
            error!("'ripple_context' key not found in params map");
            return None;
        }
    };
    let params = match serde_json::from_value::<String>(ripple_context_value) {
        Ok(p) => p,
        Err(e) => {
            error!("Failed to parse ripple_context value as String: {}", e);
            return None;
        }
    };
    let msg = match std::str::from_utf8(params.as_bytes()) {
        Ok(m) => m,
        Err(e) => {
            error!("Failed to convert params to UTF-8 string: {}", e);
            return None;
        }
    };
    let result = match serde_json::from_str::<RippleContext>(msg) {
        Ok(r) => r,
        Err(e) => {
            error!("Failed to parse RippleContext from string: {}", e);
            return None;
        }
    };
    debug!("[REFRESH TOKEN] received parsed rpc result: {:?}", result);
    Some(result)
}

async fn handle_ripple_context(state: &BadgerState, ripple_context: RippleContext) {
    if let Some(RippleContextUpdateType::TokenChanged) = ripple_context.update_type {
        if let Some(ActivationStatus::AccountToken(_t)) = &ripple_context.activation_status {
            match ThunderSessionService::get_account_session(state.get_state()).await {
                Ok(session) => {
                    debug!(
                        "[REFRESH TOKEN] received context event update account session: {:?}",
                        session
                    );
                    state.update_account_session(Some(session));
                }
                Err(e) => {
                    debug!("Error getting account session: {:?}", e);
                }
            }
        }
    }
}

fn init_extn_channel() -> ExtnChannel {
    let log_lev = ripple_sdk::log::LevelFilter::Debug;
    let _ = init_and_configure_logger(
        SEMVER_LIGHTWEIGHT,
        EXTN_NAME.into(),
        Some(vec![
            ("extn_manifest".to_string(), log_lev),
            ("device_manifest".to_string(), log_lev),
        ]),
    );

    ExtnChannel { start }
}

export_extn_channel!(ExtnChannel, init_extn_channel);
