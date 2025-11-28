use async_trait::async_trait;
use serde::Deserialize;
use serde_json::Value;
use tokio::sync::{mpsc::Sender, oneshot};

use crate::{
    gateway::appsanity_gateway::AppsanityConfig,
    service::xvp_sync_and_monitor::SyncAndMonitorModule,
    state::distributor_state::DistributorState,
    sync_and_monitor::{
        privacy_sync_monitor::PrivacySyncMonitorService,
        user_grants_sync_monitor::UserGrantsSyncMonitorService,
    },
};
use thunder_ripple_sdk::ripple_sdk::api::distributor::distributor_privacy::PrivacyResponse;
use thunder_ripple_sdk::ripple_sdk::api::session::AccountSession;

#[derive(Debug)]
pub struct ConnectParam {
    pub dev_id: String,
    pub sat: String,
    pub config: LinchpinConfig,
}

use super::{
    cloud_linchpin_monitor::{LinchpinConfig, LinchpinConnectionStatus, LinchpinErrors},
    sync_settings::SyncSettings,
};
#[derive(Debug)]
pub enum LinchpinProxyCommand {
    Connect(ConnectParam),
    Subscribe(String, oneshot::Sender<Result<(), LinchpinErrors>>), // Topic to Subscribe
    Unsubscribe(String, oneshot::Sender<Result<(), LinchpinErrors>>), //Topic to Unsubscribe
}

#[derive(Debug)]
pub enum ConvertError {
    NoKeyPresent,
    KeyHasNoValue,
    GenericError,
}

#[derive(Debug)]
pub enum StateRequest {
    AddListener(SyncSettings),
    RemoveListener(SyncSettings),
    AddPendingTopic(String),
    SetLinchpinConnectionStatus(LinchpinConnectionStatus),
    GetLinchpinConnectionStatus(oneshot::Sender<LinchpinConnectionStatus>),
    GetListeningTopics(oneshot::Sender<Vec<String>>),
    GetListenersForProperties(String, Vec<String>, oneshot::Sender<Vec<SyncSettings>>), //topic, property, cb
    GetListenersForModule(
        String,
        SyncAndMonitorModule,
        oneshot::Sender<Vec<SyncSettings>>,
    ),
    GetAllPendingTopics(oneshot::Sender<Vec<String>>),
    GetDistributorToken(oneshot::Sender<String>),
    ClearPendingTopics(String),
}

#[derive(Debug, Deserialize, Clone)]
pub struct EventPayload {
    //environment: String,
    pub settings: Value,
}

#[derive(Debug, Deserialize, Clone)]
pub struct LinchpinPayload {
    pub event_payload: EventPayload,
    //timestamp: u64,
    //event_schema: String,
    //event_id: String,
    //account_id: String,
    //partner_id: String,
    //source: String,
}

#[async_trait]
pub trait SyncAndMonitorProcessor {
    fn get_properties(&self) -> Vec<String>;
}

pub fn replace_uri_variables(base: &str, session: &AccountSession) -> String {
    let mut new_str = base.to_owned();
    new_str = new_str.replace("{partnerId}", &session.id);
    new_str = new_str.replace("{accountId}", &session.account_id);
    new_str = new_str.replace("{clientId}", "ripple");
    new_str
}

pub fn get_request_processor(
    module: &SyncAndMonitorModule,
    appsanity_config: &AppsanityConfig,
) -> Box<dyn SyncAndMonitorProcessor> {
    match module {
        SyncAndMonitorModule::Privacy => Box::new(PrivacySyncMonitorService::new()),
        SyncAndMonitorModule::UserGrants => Box::new(UserGrantsSyncMonitorService::new(
            &appsanity_config.cloud_firebolt_mapping,
        )),
    }
}

pub fn get_sync_settings(
    module: &SyncAndMonitorModule,
    state: &DistributorState,
    callback: Sender<PrivacyResponse>,
) -> SyncSettings {
    let appsanity_config = state.config.clone();
    let request_handler: Box<dyn SyncAndMonitorProcessor> =
        get_request_processor(module, &appsanity_config);
    // safe unwrap all cloud sync services are started after account session
    let session = state.get_account_session().unwrap();
    let privacy_service = state.privacy_service.clone();
    let mut sync_settings = SyncSettings {
        module: module.to_owned(),
        session,
        cloud_service_url: Default::default(),
        cloud_sync_ttl: Default::default(),
        cloud_monitor_topic: Default::default(),
        settings: Default::default(),
        callback,
        privacy_service: (*privacy_service).clone(),
    };
    match module {
        SyncAndMonitorModule::UserGrants | SyncAndMonitorModule::Privacy => {
            sync_settings.cloud_service_url = replace_uri_variables(
                &appsanity_config.privacy_service.url,
                &sync_settings.session,
            );
            sync_settings.cloud_monitor_topic = replace_uri_variables(
                &appsanity_config
                    .get_linchpin_topic_for_url(&appsanity_config.privacy_service.url)
                    .unwrap_or_default(),
                &sync_settings.session,
            );
            sync_settings.cloud_sync_ttl = appsanity_config
                .get_ttl_for_url(&appsanity_config.privacy_service.url)
                .unwrap_or_default();
            sync_settings.settings = request_handler.get_properties().clone();
        }
    }
    sync_settings
}
