use crate::manager::data_governor::DataGovernance;
use crate::manager::privacy_response_manager::PrivacyResponseManager;
use crate::message::DpabError;
use crate::state::distributor_state::DistributorState;
use crate::util::cloud_linchpin_monitor::LinchpinConnectionStatus;
use crate::util::cloud_sync_monitor_utils::get_sync_settings;
use crate::util::{
    cloud_linchpin_monitor::CloudLinchpinMonitor, cloud_periodic_sync::CloudPeriodicSync,
    cloud_sync_monitor_utils::StateRequest, sync_settings::SyncSettings,
};
use thunder_ripple_sdk::ripple_sdk::api::device::device_info_request::DeviceInfoRequest;
use thunder_ripple_sdk::ripple_sdk::api::distributor::distributor_privacy::PrivacyResponse;
use thunder_ripple_sdk::ripple_sdk::api::manifest::device_manifest::PrivacySettingsStorageType;
use thunder_ripple_sdk::ripple_sdk::api::session::AccountSession;

// #[cfg(feature = "thunder_linchpin_client")]
use std::collections::{HashMap, HashSet};
use std::sync::{Arc, Once, RwLock};
use std::time::Duration;
use thunder_ripple_sdk::ripple_sdk::extn::client::extn_client::ExtnClient;
use thunder_ripple_sdk::ripple_sdk::log::{debug, error};
use tokio::sync::mpsc::{self, Receiver, Sender};

static START_PARTNER_EXCLUSION_SYNC_THREAD: Once = Once::new();

#[derive(Debug, Clone, PartialEq)]
pub enum SyncAndMonitorModule {
    Privacy,
    UserGrants,
}

#[derive(Clone)]
pub struct SyncAndMonitorService {
    pub cloud_periodic_sync: CloudPeriodicSync,
    cloud_linchpin_monitor: CloudLinchpinMonitor,
    state: DistributorState,
}

impl SyncAndMonitorService {
    pub async fn process_state_requests(
        extn_client: ExtnClient,
        mut rx: Receiver<StateRequest>,
        account_session: Arc<RwLock<Option<AccountSession>>>,
    ) {
        debug!("Starting process state requests thread");
        let mut listener_map: HashMap<String, HashSet<SyncSettings>> = HashMap::new();
        let mut pending_topics: HashSet<String> = HashSet::new();
        let mut linchpin_connected = LinchpinConnectionStatus::Disconnected;
        while let Some(request) = rx.recv().await {
            match request {
                StateRequest::AddListener(listener) => {
                    let topic = listener.cloud_monitor_topic.to_owned();
                    debug!("Adding listener {:?} to topic: {}", listener, topic);
                    if let Some(listener_set) = listener_map.get_mut(topic.as_str()) {
                        listener_set.insert(listener);
                    } else {
                        listener_map.insert(topic, vec![listener].into_iter().collect());
                    }
                }
                StateRequest::RemoveListener(listener) => {
                    let topic = listener.cloud_monitor_topic.to_owned();
                    if let Some(listener_set) = listener_map.get_mut(topic.as_str()) {
                        listener_set.remove(&listener);
                    }
                }
                StateRequest::AddPendingTopic(topic) => {
                    pending_topics.insert(topic);
                }
                StateRequest::GetListenersForProperties(topic, property_list, callback) => {
                    debug!(
                        "Getting listener for topic: {} and property: {:?}",
                        topic, property_list
                    );
                    let mut listener_list: Vec<SyncSettings> = vec![];
                    if let Some(listener_set) = listener_map.get(topic.as_str()) {
                        for listener in listener_set {
                            if property_list
                                .iter()
                                .all(|elem| listener.settings.contains(elem))
                            {
                                listener_list.push(listener.to_owned());
                            }
                        }
                    }
                    debug!("for properties listener list: {:?}", listener_list);
                    let _ = callback.send(listener_list);
                }
                StateRequest::GetAllPendingTopics(callback) => {
                    let _ = callback.send(
                        pending_topics
                            .clone()
                            .into_iter()
                            .collect::<Vec<String>>()
                            .into(),
                    );
                }
                StateRequest::ClearPendingTopics(topic) => {
                    pending_topics.retain(|curr_topic| !curr_topic.eq_ignore_ascii_case(&topic));
                }
                StateRequest::GetListeningTopics(callback) => {
                    let _res = callback.send(listener_map.keys().cloned().collect());
                }
                StateRequest::SetLinchpinConnectionStatus(connected) => {
                    linchpin_connected = connected;
                    if linchpin_connected == LinchpinConnectionStatus::Disconnected {
                        debug!("Sending refresh context request since linchpin is disconnected");
                        if let Err(err) = extn_client
                            .request_transient(DeviceInfoRequest::InternetConnectionStatus)
                        {
                            error!("Error sending context refresh request: {:?}", err);
                        }
                    }
                }
                StateRequest::GetLinchpinConnectionStatus(callback) => {
                    let _res = callback.send(linchpin_connected.clone());
                }
                StateRequest::GetListenersForModule(topic, sync_module, callback) => {
                    debug!("Getting all listeners for module: {:?}", sync_module);
                    let mut listener_list: Vec<SyncSettings> = vec![];
                    if let Some(listener_set) = listener_map.get(topic.as_str()) {
                        for listener in listener_set {
                            if listener.module == sync_module {
                                listener_list.push(listener.to_owned());
                            }
                        }
                        let listener_set: HashSet<SyncSettings> =
                            listener_list.into_iter().collect();
                        listener_list = listener_set.iter().cloned().collect();
                    }
                    debug!("for module listener list: {:?}", listener_list);
                    let _ = callback.send(listener_list);
                }
                StateRequest::GetDistributorToken(callback) => {
                    if let Some(token) = { account_session.read().unwrap().clone() } {
                        let _ = callback.send(token.token);
                    }
                }
            }
        }
        error!("Exiting state request thread");
    }

    pub fn start(state: DistributorState) {
        let service = Self::new(&state);
        service.sync();
    }

    fn new(state: &DistributorState) -> Self {
        let (state_tx, state_rx) = mpsc::channel(32);
        let cloud_periodic_sync = CloudPeriodicSync::start(state, state_tx.clone());
        let extn_client = state.get_client();
        let cloud_linchpin_monitor = CloudLinchpinMonitor::start(extn_client.clone(), state_tx);
        let extn_client_c = extn_client.clone();
        let account_session = state.account_session.clone();
        tokio::spawn(async move {
            Self::process_state_requests(extn_client_c, state_rx, account_session).await;
        });

        SyncAndMonitorService {
            cloud_periodic_sync,
            cloud_linchpin_monitor,
            state: state.clone(),
        }
    }

    fn sync(&self) {
        let dist_state = self.state.clone();
        if dist_state
            .device_manifest
            .configuration
            .features
            .privacy_settings_storage_type
            != PrivacySettingsStorageType::Sync
        {
            return;
        }

        let callback = PrivacyResponseManager::start(dist_state.clone());
        let service = self.clone();
        tokio::spawn(async move {
            let _ = service
                .handle_sync_request(SyncAndMonitorModule::Privacy, Some(callback.clone()))
                .await;

            let _ = service
                .handle_sync_request(SyncAndMonitorModule::UserGrants, Some(callback.clone()))
                .await;

            sync_partner_exclusions(dist_state).await;
        });
    }

    async fn handle_sync_request(
        &self,
        module: SyncAndMonitorModule,
        callback: Option<Sender<PrivacyResponse>>,
    ) -> Result<(), DpabError> {
        if let Some(dist_session) = self.state.get_account_session() {
            if let Some(callback) = callback {
                let sync_setting = get_sync_settings(&module, &self.state, callback.to_owned());
                if !sync_setting.cloud_monitor_topic.is_empty() {
                    self.cloud_linchpin_monitor
                        .subscribe(
                            sync_setting.clone(),
                            self.state.config.linchpin_config.clone(),
                            &dist_session.device_id,
                            &dist_session.token,
                        )
                        .await;
                } else {
                    debug!(
                        "service: {} Not configured with listen topic so not changes from linchpin",
                        sync_setting.cloud_service_url
                    );
                }
                // Periodically fetch from XVP
                if sync_setting.cloud_sync_ttl > 0 {
                    self.cloud_periodic_sync.sync(sync_setting).await;
                    return Ok(());
                } else {
                    debug!(
                        "service: {} Not configured with TTL so not starting periodic sync",
                        sync_setting.cloud_service_url
                    );
                }
            }
        }
        return Err(DpabError::NotDataFound);
    }
}

async fn sync_partner_exclusions(state: DistributorState) {
    let state_for_exclusion = state.clone();
    START_PARTNER_EXCLUSION_SYNC_THREAD.call_once(|| {
        debug!("Starting partner exclusion sync thread");
        tokio::spawn(async move {
            let duration = state
                .device_manifest
                .configuration
                .partner_exclusion_refresh_timeout
                .into();
            let mut interval = tokio::time::interval(Duration::from_secs(duration));
            loop {
                DataGovernance::refresh_partner_exclusions(&state_for_exclusion).await;
                interval.tick().await;
            }
        });
    });
}
