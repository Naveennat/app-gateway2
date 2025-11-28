use std::sync::{Arc, RwLock};

use super::sift_metrics_state::SiftMetricsState;
use crate::rpc::eos_authentication_rpc::TokenResult;
use crate::service::appsanity_metrics::initiate_metrics;
use crate::service::appsanity_privacy::PrivacyService;
use crate::service::ott_token::OttTokenService;
use crate::service::thor_permission::ThorPermissionService;
use crate::service::xvp_sync_and_monitor::SyncAndMonitorService;
use crate::{
    gateway::appsanity_gateway::AppsanityConfig, manager::data_governor::DataGovernanceState,
};
use ripple_sdk::service::service_client::ServiceClient;
use serde_json::Value;
use thunder_ripple_sdk::client::thunder_client::ThunderClient;
use thunder_ripple_sdk::processors::thunder_persistent_store::ThunderStorageRequestProcessor;
use thunder_ripple_sdk::ripple_sdk::api::apps::AppEvent;
use thunder_ripple_sdk::ripple_sdk::api::config::FEATURE_DISTRIBUTOR_SESSION;
use thunder_ripple_sdk::ripple_sdk::api::context::FeatureUpdate;
use thunder_ripple_sdk::ripple_sdk::api::context::RippleContextUpdateRequest;
use thunder_ripple_sdk::ripple_sdk::api::device::device_peristence::SetStorageProperty;
use thunder_ripple_sdk::ripple_sdk::api::device::device_peristence::{
    DeleteStorageProperty, GetStorageProperty,
};
use thunder_ripple_sdk::ripple_sdk::api::device::device_request::AccountToken;
use thunder_ripple_sdk::ripple_sdk::api::distributor::distributor_privacy::ExclusionPolicy;
use thunder_ripple_sdk::ripple_sdk::api::distributor::distributor_privacy::PrivacySettings;
use thunder_ripple_sdk::ripple_sdk::api::distributor::distributor_privacy::SetPropertyParams;
use thunder_ripple_sdk::ripple_sdk::api::gateway::rpc_gateway_api::RpcRequest;
use thunder_ripple_sdk::ripple_sdk::api::manifest::device_manifest::DefaultValues;
use thunder_ripple_sdk::ripple_sdk::api::manifest::device_manifest::DeviceManifest;
use thunder_ripple_sdk::ripple_sdk::api::ripple_cache::RippleCache;
use thunder_ripple_sdk::ripple_sdk::api::session::AccountSession;
use thunder_ripple_sdk::ripple_sdk::api::storage_manager::IStorageOperator;
use thunder_ripple_sdk::ripple_sdk::async_trait::async_trait;
use thunder_ripple_sdk::ripple_sdk::extn::client::extn_client::ExtnClient;
use thunder_ripple_sdk::ripple_sdk::extn::extn_client_message::ExtnResponse;
use thunder_ripple_sdk::ripple_sdk::framework::RippleResponse;
use thunder_ripple_sdk::ripple_sdk::utils::error::RippleError;
use thunder_ripple_sdk::thunder_state::ThunderState;

#[derive(Clone)]
pub struct DistributorState {
    pub device_manifest: Arc<DeviceManifest>,
    pub config: Arc<AppsanityConfig>,
    pub data_governance: Arc<DataGovernanceState>,
    pub metrics: Arc<SiftMetricsState>,
    pub thunder: Arc<ThunderState>,
    pub privacy: Arc<RwLock<PrivacySettings>>,
    pub cache: Arc<RippleCache>,
    pub account_session: Arc<RwLock<Option<AccountSession>>>,
    pub root_device_token: Arc<RwLock<Option<TokenResult>>>,
    pub privacy_service: Arc<PrivacyService>,
    pub auth_service: Box<AuthService>,
    pub service_client: Option<ServiceClient>,
}

#[derive(Clone)]
pub struct AuthService {
    pub(crate) thor_permission_service: Arc<ThorPermissionService>,
    pub(crate) ott_token_service: Arc<OttTokenService>,
}

impl AuthService {
    pub(crate) fn new(
        thor_permission_service: ThorPermissionService,
        ott_token_service: OttTokenService,
    ) -> Box<Self> {
        Box::new(AuthService {
            thor_permission_service: Arc::new(thor_permission_service),
            ott_token_service: Arc::new(ott_token_service),
        })
    }

    pub(crate) fn get_ott_token_service(&self) -> Arc<OttTokenService> {
        self.ott_token_service.clone()
    }

    pub(crate) fn get_thor_permission_service(&self) -> Arc<ThorPermissionService> {
        self.thor_permission_service.clone()
    }
}

impl DistributorState {
    pub fn new(
        device_manifest: &DeviceManifest,
        thunder: ThunderState,
        config: AppsanityConfig,
        auth_service: Box<AuthService>,
        service_client: Option<ServiceClient>,
    ) -> Self {
        Self {
            device_manifest: Arc::new(device_manifest.clone()),
            metrics: Arc::new(SiftMetricsState::new(
                config.clone(),
                thunder.clone(),
                service_client.clone(),
            )),
            data_governance: Arc::new(DataGovernanceState::default()),
            thunder: Arc::new(thunder),
            privacy_service: Arc::new(PrivacyService::new(
                config.clone().privacy_service.url,
                &config.cloud_firebolt_mapping,
            )),
            config: Arc::new(config),
            privacy: Arc::new(RwLock::new(PrivacySettings::default())),
            cache: Arc::new(RippleCache::default()),
            account_session: Arc::new(RwLock::new(None)),
            auth_service,
            root_device_token: Arc::new(RwLock::new(None)),
            service_client,
        }
    }

    pub fn get_client(&self) -> ExtnClient {
        self.thunder.get_client()
    }

    pub fn get_service_client(&self) -> Option<ServiceClient> {
        self.service_client.clone()
    }

    pub fn get_thunder(&self) -> ThunderState {
        (*self.thunder).clone()
    }

    pub fn get_thunder_client(&self) -> ThunderClient {
        self.thunder.get_thunder_client().clone()
    }

    pub fn get_auth_service(&self) -> Box<AuthService> {
        self.auth_service.clone()
    }

    pub fn get_root_device_token(&self) -> Option<TokenResult> {
        let root_device_token = self.root_device_token.read().unwrap();
        root_device_token.clone()
    }

    pub fn update_root_device_token(&self, token: TokenResult) {
        let mut root_device_token = self.root_device_token.write().unwrap();
        *root_device_token = Some(token);
    }

    pub fn update_privacy(&self, data: PrivacySettings) {
        self.cache.update_all_privacy_cache(&data);
        {
            let mut privacy = self.privacy.write().unwrap();
            privacy.update(data);
        }
    }

    pub fn update_privacy_setting(&self, data: SetPropertyParams) {
        let value = data.value.clone();
        if let Ok(property) = data.setting.clone().try_into() {
            self.cache
                .update_cached_bool_storage_property(self, &property, value);
        }
        {
            let mut privacy = self.privacy.write().unwrap();
            privacy.update_privacy_setting(data.setting, data.value);
        }
    }

    pub fn update_account_session(&self, ref_session: &AccountSession) {
        let mut session = self.account_session.write().unwrap();
        let _ = session.insert(ref_session.clone());
    }

    pub fn update_token(&self, token: String) {
        if let Some(session_value) = self.account_session.write().unwrap().as_mut() {
            session_value.token = token;
        }
    }

    pub fn get_account_session(&self) -> Option<AccountSession> {
        self.account_session.read().unwrap().clone()
    }

    pub async fn get_partner_exclusions(&self) -> Result<ExclusionPolicy, RippleError> {
        if let Some(session) = self.get_account_session() {
            return if let Ok(v) = self.privacy_service.get_partner_exclusions(session).await {
                Ok(v)
            } else {
                Err(RippleError::ServiceError)
            };
        }

        Err(RippleError::ServiceError)
    }

    pub fn handle_first_account_session_update(&self, session: &AccountSession) {
        let _ = self
            .get_client()
            .request_transient(RippleContextUpdateRequest::Token(AccountToken {
                token: session.token.clone(),
                expires: 0,
            }));

        if session.id.eq_ignore_ascii_case("comcast") {
            let _ =
                self.get_client()
                    .request_transient(RippleContextUpdateRequest::UpdateFeatures(vec![
                        FeatureUpdate::new(FEATURE_DISTRIBUTOR_SESSION.to_string(), true),
                    ]));
        }
        SyncAndMonitorService::start(self.clone());
        initiate_metrics(self.clone());
    }

    pub fn get_privacy(&self) -> PrivacySettings {
        self.privacy.read().unwrap().clone()
    }
}

#[async_trait]
impl IStorageOperator for DistributorState {
    fn get_cache(&self) -> RippleCache {
        (*self.cache).clone()
    }

    fn get_default(&self) -> DefaultValues {
        self.device_manifest.configuration.default_values.clone()
    }

    async fn persist(&self, request: SetStorageProperty) -> RippleResponse {
        if ThunderStorageRequestProcessor::set_in_peristent_store(&self.thunder, request)
            .await
            .is_ok()
        {
            Ok(())
        } else {
            Err(RippleError::InvalidInput)
        }
    }

    async fn pull(&self, request: GetStorageProperty) -> Result<ExtnResponse, RippleError> {
        ThunderStorageRequestProcessor::get_value_in_persistent_store(&self.thunder, request).await
    }

    async fn delete(&self, request: DeleteStorageProperty) -> RippleResponse {
        if let Err(e) =
            ThunderStorageRequestProcessor::delete_key_in_persistent_store(&self.thunder, request)
                .await
        {
            Err(e)
        } else {
            Ok(())
        }
    }

    fn emit(&self, event_name: &str, result: &Value, context: Option<Value>) {
        let app_event = serde_json::to_value(AppEvent {
            event_name: event_name.to_owned(),
            result: result.clone(),
            context,
            app_id: None,
        })
        .unwrap();
        let _ = self
            .get_client()
            .request_transient(RpcRequest::get_new_internal(
                "ripple.sendAppEvent".to_owned(),
                Some(app_event),
            ));
    }

    fn on_privacy_updated(&self, cache: RippleCache) {
        self.metrics.update_data_governance_tags(self, &cache);
    }
}

#[cfg(test)]
pub mod tests {
    use ripple_sdk::api::manifest::device_manifest::DeviceManifest;
    use thunder_ripple_sdk::{
        ripple_sdk::Mockable, tests::mock_thunder_controller::MockThunderController,
    };

    use crate::{
        gateway::appsanity_gateway::AppsanityConfig,
        service::{ott_token::OttTokenService, thor_permission::ThorPermissionService},
    };

    use super::{AuthService, DistributorState};

    impl Mockable for DistributorState {
        fn mock() -> Self {
            let device_manifest = DeviceManifest::default();
            let config = AppsanityConfig::mock();
            let thunder = MockThunderController::get_thunder_state_mock();
            let auth_service =
                AuthService::new(ThorPermissionService::mock(), OttTokenService::mock());
            DistributorState::new(&device_manifest, thunder, config, auth_service, None)
        }
    }
}
