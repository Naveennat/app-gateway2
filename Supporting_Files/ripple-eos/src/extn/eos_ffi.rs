use crate::extn::thunder::thunder_session::ThunderSessionService;
use crate::manager::context_manager::ContextManager;
use crate::manager::session_manager::handle_session;
use crate::model::permissions::thor_permission_registry::ThorPermissionRegistry;
use crate::rpc::setup_distributor_rpc_handlers::{
    get_rpc_methods_for_service, setup_distributor_rpc_handlers,
};
use crate::state::distributor_state::{AuthService, DistributorState};
use ripple_sdk::api::manifest::device_manifest::DeviceManifest;
use ripple_sdk::api::manifest::extn_manifest::ExtnManifest;
use ripple_sdk::api::manifest::extn_manifest::ExtnSymbol;
use ripple_sdk::api::manifest::ripple_manifest_loader::RippleManifestLoader;
use ripple_sdk::service::service_client::ServiceClient;
use ripple_sdk::utils::logger::init_and_configure_logger;
use ripple_sdk::{
    export_extn_channel,
    extn::{
        extn_id::{ExtnClassId, ExtnId},
        ffi::ffi_channel::ExtnChannel,
    },
    log::{debug, error, info},
    utils::extn_utils::ExtnUtils,
};

use thunder_ripple_sdk::{
    bootstrap::boot_thunder::boot_thunder,
    client::plugin_manager::{ThunderPluginBootParam, ThunderPluginParam},
};

use crate::{
    extn::{
        appsanity_permission_processor::DistributorPermissionProcessor,
        appsanity_privacy_processor::DistributorPrivacyProcessor,
        appsanity_usergrants_processor::DistributorStoreUserGrantsProcessor,
        extn_utils::DistributorConfig,
    },
    service::{ott_token::OttTokenService, thor_permission::ThorPermissionService},
    util::service_util::create_grpc_client_session,
};

include!(concat!(env!("OUT_DIR"), "/version.rs"));

const EXTN_NAME: &'static str = "eos";
pub const EOS_DISTRIBUTOR_SERVICE_ID: &str = "ripple:channel:distributor:eos";

pub async fn start_service() {
    let log_lev = thunder_ripple_sdk::ripple_sdk::log::LevelFilter::Debug;
    let _ = init_and_configure_logger(
        SEMVER_LIGHTWEIGHT,
        EXTN_NAME.into(),
        Some(vec![
            ("extn_manifest".to_string(), log_lev),
            ("device_manifest".to_string(), log_lev),
        ]),
    );
    info!("Starting EOS channel");
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
    info!("Starting EOS channel");
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
    let runtime = ExtnUtils::get_runtime("e-eos".to_owned(), service_client.get_stack_size());

    runtime.block_on(async move {
        init(service_client.clone(), device_manifest).await;
    });
}

fn get_symbol(extn_manifest: ExtnManifest) -> Option<ExtnSymbol> {
    let extn = ExtnId::new_channel(ExtnClassId::Distributor, EXTN_NAME.to_string()).to_string();
    extn_manifest.get_extn_symbol(&extn)
}

pub async fn init(client: ServiceClient, device_manifest: DeviceManifest) {
    // Return error if service_client is not holding an extn client internally.
    // This will be fixed when we remove extension client completely.
    if let Some(mut extn_client) = client.get_extn_client() {
        let client_c_for_spawn = client.clone();
        let client_c_for_init = client.clone();

        let config =
            DistributorConfig::get(device_manifest.configuration.distributor_services.clone());

        let config_c = config.clone();
        let permission_url = config_c.permission_service.url;
        let ott_channel_url = config_c.ott_token_service.url;
        let device_manifest_c = device_manifest.clone();

        tokio::spawn(async move {
            debug!("Starting thunder client from eos distributor");
            let bootstrap_state = boot_thunder(
                extn_client.clone(),
                ThunderPluginBootParam {
                    activate_on_boot: ThunderPluginParam::Default,
                    expected: ThunderPluginParam::Default,
                },
                &device_manifest_c,
            )
            .await;

            let ots = OttTokenService::new_from(create_grpc_client_session(ott_channel_url));
            let tps = ThorPermissionService::new(permission_url, ThorPermissionRegistry::new());

            let distributor_state = if let Some(state) = bootstrap_state {
                let thunder_state = state.state;
                let aas = AuthService::new(tps.clone(), ots.clone());
                let dist_state = DistributorState::new(
                    &device_manifest_c,
                    thunder_state.clone(),
                    config.clone(),
                    aas,
                    Some(client),
                );
                extn_client.add_request_processor(ThunderSessionService::new(thunder_state));
                dist_state
            } else {
                error!("Error booting thunder");
                return;
            };

            let _ = client_c_for_spawn
                .clone()
                .set_service_rpc_route(get_rpc_methods_for_service(&distributor_state));
            setup_distributor_rpc_handlers(distributor_state.clone());

            let perm_proc =
                DistributorPermissionProcessor::new(extn_client.clone(), Box::new(tps.clone()));
            extn_client.add_request_processor(perm_proc);

            let privacy_proc = DistributorPrivacyProcessor::new(distributor_state.clone());
            extn_client.add_request_processor(privacy_proc);

            let usergrants_proc =
                DistributorStoreUserGrantsProcessor::new(distributor_state.clone());
            extn_client.add_request_processor(usergrants_proc);

            ContextManager::setup(distributor_state.clone());
            handle_session(distributor_state.clone()).await;
        });

        client_c_for_init.initialize().await;
    } else {
        error!("Service client does not hold an extn client. Cannot start eos extension.");
        return;
    }
}

fn init_extn_channel() -> ExtnChannel {
    let log_lev = thunder_ripple_sdk::ripple_sdk::log::LevelFilter::Debug;
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
