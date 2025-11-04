use crate::{message::DpabError, service::xvp_sync_and_monitor::SyncAndMonitorModule};
use std::hash::{Hash, Hasher};
use tokio::sync::{mpsc::Sender, oneshot};

use super::cloud_sync_monitor_utils::StateRequest;
use crate::service::appsanity_privacy::PrivacyService;
use thunder_ripple_sdk::ripple_sdk::{
    api::distributor::distributor_privacy::PrivacyResponse, api::session::AccountSession,
};
#[derive(Debug, Clone)]
pub struct SyncSettings {
    pub module: SyncAndMonitorModule,
    pub session: AccountSession,
    pub cloud_service_url: String,
    pub cloud_sync_ttl: u32,
    pub cloud_monitor_topic: String,
    pub settings: Vec<String>,
    pub callback: Sender<PrivacyResponse>,
    pub privacy_service: PrivacyService,
}

impl Eq for SyncSettings {}

impl PartialEq for SyncSettings {
    fn eq(&self, other: &SyncSettings) -> bool {
        self.cloud_monitor_topic == other.cloud_monitor_topic && self.settings.eq(&other.settings)
    }
}

impl Hash for SyncSettings {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.settings.hash(state);
        self.cloud_monitor_topic.hash(state);
    }
}

impl SyncSettings {
    pub async fn get_values_from_cloud(
        &self,
        state_tx: Sender<StateRequest>,
    ) -> Result<PrivacyResponse, DpabError> {
        let (tx, rx) = oneshot::channel();
        let _ = state_tx.send(StateRequest::GetDistributorToken(tx)).await;
        let mut session = self.session.clone();
        let res_token = rx.await;
        if res_token.is_err() {
            return Err(DpabError::ServiceError);
        }
        let token = res_token.unwrap();
        session.token = token.to_owned();
        match self.module {
            SyncAndMonitorModule::Privacy => {
                let result = self.privacy_service.get_properties(session.clone()).await;
                result.map_err(|_e| DpabError::ServiceError)
            }
            SyncAndMonitorModule::UserGrants => {
                let result = self.privacy_service.get_user_grants(session.clone()).await;
                result.map_err(|_e| DpabError::ServiceError)
            }
        }
    }
}
