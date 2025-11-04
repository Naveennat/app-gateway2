use std::{
    collections::HashMap,
    sync::{Arc, RwLock},
};

use crate::{
    model::metrics::{
        AppDataGovernanceState, BehavioralMetricContext, BehavioralMetricPayload, MetricsContext,
    },
    rpc::eos_device_rpc::get_mac,
};
use env_file_reader::read_file;
use log::trace;
use ripple_sdk::service::service_client::ServiceClient;
use thunder_ripple_sdk::ripple_sdk::api::gateway::rpc_gateway_api::RpcRequest;
use thunder_ripple_sdk::ripple_sdk::api::ripple_cache::RippleCache;
use thunder_ripple_sdk::ripple_sdk::api::storage_manager::StorageManager;
use thunder_ripple_sdk::ripple_sdk::utils::serde_utils::SerdeClearString;
use thunder_ripple_sdk::ripple_sdk::{
    api::gateway::rpc_gateway_api::CallContext, framework::RippleResponse,
};
use thunder_ripple_sdk::ripple_sdk::{
    api::{
        device::device_info_request::FirmwareInfo,
        distributor::distributor_privacy::{DataEventType, PrivacySettingsData},
        manifest::device_manifest::DataGovernanceConfig,
        storage_property::StorageProperty,
    },
    chrono::{DateTime, Utc},
    extn::extn_client_message::ExtnResponse,
    log::{debug, error, info, warn},
    utils::error::RippleError,
};
use thunder_ripple_sdk::{
    processors::thunder_device_info::ThunderDeviceInfoRequestProcessor,
    ripple_sdk::api::firebolt::fb_lifecycle_management::CompletedSessionResponse,
};
use thunder_ripple_sdk::{ripple_sdk::api::apps::AppError, thunder_state::ThunderState};

use rand::Rng;
use tokio::sync::mpsc;
use uuid::Uuid;

use crate::{
    gateway::appsanity_gateway::AppsanityConfig,
    manager::data_governor::DataGovernance,
    model::metrics::IUpdateContext,
    service::appsanity_metrics::{ContextualMetricsService, SiftService},
};

use super::distributor_state::DistributorState;

include!(concat!(env!("OUT_DIR"), "/version.rs"));

const PERSISTENT_STORAGE_NAMESPACE: &str = "accountProfile";
const PERSISTENT_STORAGE_KEY_PROPOSITION: &str = "proposition";
const PERSISTENT_STORAGE_KEY_RETAILER: &str = "retailer";
const PERSISTENT_STORAGE_KEY_PRIMARY_PROVIDER: &str = "jvagent";
const PERSISTENT_STORAGE_KEY_COAM: &str = "coam";
const PERSISTENT_STORAGE_KEY_ACCOUNT_TYPE: &str = "accountType";
const PERSISTENT_STORAGE_KEY_OPERATOR: &str = "operator";
const PERSISTENT_STORAGE_ACCOUNT_DETAIL_TYPE: &str = "detailType";
const PERSISTENT_STORAGE_ACCOUNT_DEVICE_TYPE: &str = "deviceType";
const PERSISTENT_STORAGE_ACCOUNT_DEVICE_MANUFACTURER: &str = "deviceManufacturer";

#[derive(Debug, Clone)]
pub struct DeviceSessionIdentifier {
    pub device_session_id: Uuid,
}

impl Default for DeviceSessionIdentifier {
    fn default() -> Self {
        Self {
            device_session_id: Uuid::new_v4(),
        }
    }
}
impl From<DeviceSessionIdentifier> for String {
    fn from(device_session_identifier: DeviceSessionIdentifier) -> Self {
        device_session_identifier.device_session_id.to_string()
    }
}
impl From<String> for DeviceSessionIdentifier {
    fn from(uuid_str: String) -> Self {
        DeviceSessionIdentifier {
            device_session_id: Uuid::parse_str(&uuid_str).unwrap_or_default(),
        }
    }
}

#[derive(Debug, Clone)]
pub struct SiftAppSession {
    pub app_session_id: String,
    pub app_user_session_id: Option<String>,
    pub app_metrics_version: Option<String>,
}

#[derive(Debug, Clone)]
pub struct SiftMetricsState {
    pub start_time: DateTime<Utc>,
    pub device_session_id: DeviceSessionIdentifier,
    pub version: Option<String>,
    pub app_session: Arc<RwLock<HashMap<String, SiftAppSession>>>,
    pub service: SiftService,
    sender: mpsc::Sender<i64>,
    receiver: Arc<RwLock<Option<mpsc::Receiver<i64>>>>,
}

fn ripple_version_from_etc() -> Option<String> {
    /*
    read /etc/rippleversion
    */
    static RIPPLE_VER_FILE_DEFAULT: &str = "/etc/rippleversion.txt";
    static RIPPLE_VER_VAR_NAME_DEFAULT: &str = "RIPPLE_VER";
    let version_file_name =
        std::env::var("RIPPLE_VERSIONS_FILE").unwrap_or(RIPPLE_VER_FILE_DEFAULT.to_string());
    let version_var_name =
        std::env::var("RIPPLE_VERSIONS_VAR").unwrap_or(RIPPLE_VER_VAR_NAME_DEFAULT.to_string());

    match read_file(version_file_name.clone()) {
        Ok(env_vars) => {
            if let Some(version) = env_vars.get(&version_var_name) {
                info!(
                    "Printing ripple version from rippleversion.txt {:?}",
                    version.clone()
                );
                return Some(version.clone());
            }
        }
        Err(err) => {
            warn!(
                "error reading versions from {}, err={:?}",
                version_file_name, err
            );
        }
    }
    warn!("error reading versions from {}", version_file_name,);
    None
}

fn os_version_from_etc() -> Option<String> {
    /*
    read /etc/skyversion
    */
    static DEVICE_OS_VER_FILE_DEFAULT: &str = "/etc/skyversion.txt";
    static DEVICE_OS_VER_VAR_NAME_DEFAULT: &str = "SKY_VERSION";
    let version_file_name =
        std::env::var("DEVICE_OS_VERSION_FILE").unwrap_or(DEVICE_OS_VER_FILE_DEFAULT.to_string());
    let version_var_name = std::env::var("DEVICE_OS_VERSION_VAR")
        .unwrap_or(DEVICE_OS_VER_VAR_NAME_DEFAULT.to_string());

    match read_file(version_file_name.clone()) {
        Ok(env_vars) => {
            if let Some(version) = env_vars.get(&version_var_name) {
                info!(
                    "Printing device OS version from skyversion.txt {:?}",
                    version
                );
                return Some(version.to_owned());
            }
        }
        Err(err) => {
            warn!(
                "error reading os_versions from {}, err={:?}",
                version_file_name, err
            );
        }
    }
    warn!("error reading os_versions from {}", version_file_name);
    None
}

impl SiftMetricsState {
    pub fn new(
        config: AppsanityConfig,
        thunder: ThunderState,
        service_client: Option<ServiceClient>,
    ) -> Self {
        let (sender, tr) = mpsc::channel(2);
        Self {
            service: SiftService::new(&config, thunder, service_client.clone()),
            start_time: DateTime::default(),
            device_session_id: DeviceSessionIdentifier::default(),
            version: ripple_version_from_etc(),
            app_session: Arc::new(RwLock::new(HashMap::new())),
            sender,
            receiver: Arc::new(RwLock::new(Some(tr))),
        }
    }

    pub fn check_send(&self) {
        if self.sender.capacity() > 0 {
            if self.sender.try_send(Utc::now().timestamp_millis()).is_err() {
                trace!("existing metrics events in queue")
            }
        }
    }

    fn get_app_session(&self, app_id: &str) -> Option<SiftAppSession> {
        let app_session = self.app_session.read().unwrap();
        app_session.get(app_id).cloned()
    }

    pub fn get_context(&self) -> MetricsContext {
        self.service.get_context()
    }

    fn get_option_string(s: String) -> Option<String> {
        if !s.is_empty() {
            return Some(s);
        }
        None
    }

    pub fn update_data_governance_tags(
        &self,
        platform_state: &DistributorState,
        cache: &RippleCache,
    ) {
        fn update_tags(
            data_governance_config: &DataGovernanceConfig,
            data: Option<bool>,
            tags: &mut Vec<String>,
            data_event_type: DataEventType,
            storage_property: StorageProperty,
        ) {
            if let Some(true) = data {
                if let Some(policy) = data_governance_config.get_policy(data_event_type) {
                    if let Some(setting_tag) = policy
                        .setting_tags
                        .iter()
                        .find(|t| t.setting == storage_property)
                    {
                        for tag in setting_tag.tags.clone() {
                            tags.push(tag);
                        }
                    }
                }
            }
        }
        let privacy_settings_data: PrivacySettingsData = cache.get();
        let mut governance_tags: Vec<String> = Vec::new();
        let data_governance_config = platform_state
            .device_manifest
            .configuration
            .data_governance
            .clone();

        update_tags(
            &data_governance_config,
            privacy_settings_data.allow_business_analytics,
            &mut governance_tags,
            DataEventType::BusinessIntelligence,
            StorageProperty::AllowBusinessAnalytics,
        );

        update_tags(
            &data_governance_config,
            privacy_settings_data.allow_resume_points,
            &mut governance_tags,
            DataEventType::Watched,
            StorageProperty::AllowWatchHistory,
        );

        update_tags(
            &data_governance_config,
            privacy_settings_data.allow_personalization,
            &mut governance_tags,
            DataEventType::BusinessIntelligence,
            StorageProperty::AllowPersonalization,
        );

        update_tags(
            &data_governance_config,
            privacy_settings_data.allow_product_analytics,
            &mut governance_tags,
            DataEventType::BusinessIntelligence,
            StorageProperty::AllowProductAnalytics,
        );

        update_tags(
            &data_governance_config,
            privacy_settings_data.allow_app_content_ad_targeting,
            &mut governance_tags,
            DataEventType::BusinessIntelligence,
            StorageProperty::AllowAppContentAdTargeting,
        );

        let mut context = self.service.get_context();
        context.data_governance_tags = if !governance_tags.is_empty() {
            Some(governance_tags)
        } else {
            None
        };
        self.service.update_metrics_context(Some(context));
    }

    async fn get_persistent_store_string(
        state: &DistributorState,
        key: &'static str,
    ) -> Option<String> {
        match StorageManager::get_string_from_namespace(
            state,
            PERSISTENT_STORAGE_NAMESPACE.to_string(),
            key,
            None,
        )
        .await
        {
            Ok(resp) => Self::get_option_string(resp.as_value()),
            Err(e) => {
                error!(
                    "get_persistent_store_string: Could not retrieve value: e={:?}",
                    e
                );
                None
            }
        }
    }

    async fn get_persistent_store_bool(
        state: &DistributorState,
        key: &'static str,
    ) -> Option<bool> {
        match StorageManager::get_bool_from_namespace(
            state,
            PERSISTENT_STORAGE_NAMESPACE.to_string(),
            key,
        )
        .await
        {
            Ok(resp) => Some(resp.as_value()),
            Err(e) => {
                error!(
                    "get_persistent_store_bool: Could not retrieve value: e={:?}",
                    e
                );
                None
            }
        }
    }

    fn unset(s: &str) -> String {
        format!("{}{}", s, ".unset")
    }

    pub fn get_receiver(&self) -> Result<mpsc::Receiver<i64>, RippleError> {
        let mut receiver = self.receiver.write().unwrap();
        if let Some(t) = receiver.take() {
            Ok(t)
        } else {
            Err(RippleError::ClientMissing)
        }
    }

    pub async fn initialize(state: &mut DistributorState) {
        let metrics_percentage = state
            .device_manifest
            .configuration
            .metrics_logging_percentage;

        let random_number = rand::thread_rng().gen_range(1..101);
        let metrics_enabled = random_number <= metrics_percentage;
        let ripple_version = state
            .metrics
            .version
            .clone()
            .unwrap_or(String::from(SEMVER_LIGHTWEIGHT));
        debug!(
            "initialize: metrics_percentage={}, random_number={}, enabled={}",
            metrics_percentage, random_number, metrics_enabled
        );
        let thunder_state = state.thunder.clone();
        let mac = get_mac(&state).await;
        let mac_address: Option<String> = if mac.is_empty() { None } else { Some(mac) };

        let serial = ThunderDeviceInfoRequestProcessor::get_serial_number(&thunder_state).await;
        let serial_number: Option<String> = if serial.is_empty() {
            None
        } else {
            Some(serial)
        };

        let model = ThunderDeviceInfoRequestProcessor::model_info(&thunder_state).await;
        let device_model: Option<String> = if model.is_empty() { None } else { Some(model) };

        let mut language = "en".to_owned();
        if let Ok(v) = thunder_state
            .get_client()
            .request(RpcRequest::get_new_internal(
                "localization.language".to_owned(),
                None,
            ))
            .await
        {
            if let Some(ExtnResponse::Value(s)) = v.payload.extract() {
                language = SerdeClearString::as_clear_string(&s);
            }
        }

        let os_info: FirmwareInfo =
            ThunderDeviceInfoRequestProcessor::get_firmware_version(&thunder_state)
                .await
                .into();

        debug!("got os_info={:?}", &os_info);

        let mut os_ver = "not.set".to_owned();
        if let Some(os_version) = os_version_from_etc() {
            os_ver = SerdeClearString::as_clear_string(&os_version);
        }

        let mut device_name = "not.set".to_owned();
        if let Ok(v) = thunder_state
            .get_client()
            .request(RpcRequest::get_new_internal("device.name".to_owned(), None))
            .await
        {
            if let Some(ExtnResponse::Value(s)) = v.payload.extract() {
                device_name = SerdeClearString::as_clear_string(&s);
            }
        }

        let mut country = None;
        if let Ok(v) = thunder_state
            .get_client()
            .request(RpcRequest::get_new_internal(
                "localization.countryCode".to_owned(),
                None,
            ))
            .await
        {
            if let Some(ExtnResponse::Value(s)) = v.payload.extract() {
                let _ = country.insert(SerdeClearString::as_clear_string(&s));
            }
        }

        let firmware = os_info.name.clone();
        let activated = Some(true);

        let proposition =
            Self::get_persistent_store_string(state, PERSISTENT_STORAGE_KEY_PROPOSITION)
                .await
                .unwrap_or("Proposition.missing.from.persistent.store".into());

        let retailer =
            Self::get_persistent_store_string(state, PERSISTENT_STORAGE_KEY_RETAILER).await;

        let primary_provider =
            Self::get_persistent_store_string(state, PERSISTENT_STORAGE_KEY_PRIMARY_PROVIDER).await;

        let coam = Self::get_persistent_store_bool(state, PERSISTENT_STORAGE_KEY_COAM).await;

        let region = StorageManager::get_string(state, StorageProperty::Locality)
            .await
            .ok();

        let account_type =
            Self::get_persistent_store_string(state, PERSISTENT_STORAGE_KEY_ACCOUNT_TYPE).await;

        let operator =
            Self::get_persistent_store_string(state, PERSISTENT_STORAGE_KEY_OPERATOR).await;

        let account_detail_type =
            Self::get_persistent_store_string(state, PERSISTENT_STORAGE_ACCOUNT_DETAIL_TYPE).await;

        let device_type =
            match Self::get_persistent_store_string(state, PERSISTENT_STORAGE_ACCOUNT_DEVICE_TYPE)
                .await
            {
                Some(s) => s,
                None => state.device_manifest.get_form_factor(),
            };

        let device_manufacturer = match Self::get_persistent_store_string(
            state,
            PERSISTENT_STORAGE_ACCOUNT_DEVICE_MANUFACTURER,
        )
        .await
        {
            Some(s) => s,
            None => {
                let mut make = Self::unset("device.make");
                if let Ok(v) = thunder_state
                    .get_client()
                    .request(RpcRequest::get_new_internal("device.make".to_owned(), None))
                    .await
                {
                    if let Some(ExtnResponse::Value(s)) = v.payload.extract() {
                        make = SerdeClearString::as_clear_string(&s);
                    }
                }
                make
            }
        };

        let authenticated = Some(state.config.behavioral_metrics.sift.authenticated);

        {
            // Time to set them
            let mut context: MetricsContext = MetricsContext::new();

            context.enabled = metrics_enabled;

            if let Some(mac) = mac_address {
                context.mac_address = mac;
            }

            if let Some(sn) = serial_number {
                context.serial_number = sn;
            }

            if let Some(model) = device_model {
                context.device_model = model;
            }

            context.device_language = language;
            context.os_name = os_info.name;
            context.os_ver = os_ver;
            context.device_name = Some(device_name);
            context.device_session_id = state.metrics.device_session_id.clone().into();
            context.firmware = firmware;
            context.ripple_version = ripple_version;
            context.activated = activated;
            context.proposition = proposition;
            context.retailer = retailer;
            context.primary_provider = primary_provider;
            context.coam = coam;
            context.country = country;
            context.region = region;
            context.account_type = account_type;
            context.operator = operator;
            context.account_detail_type = account_detail_type;
            context.device_type = device_type;
            context.device_manufacturer = device_manufacturer;
            context.authenticated = authenticated;
            state.metrics.service.update_metrics_context(Some(context));
        }
        {
            Self::update_account_session(state).await;
        }
    }

    pub async fn update_account_session(state: &DistributorState) {
        {
            let mut context = state.metrics.service.get_context();
            let account_session = state.get_account_session();
            if let Some(session) = account_session {
                context.account_id = Some(session.account_id);
                context.device_id = Some(session.device_id);
                context.distribution_tenant_id = session.id;
            } else {
                context.account_id = None;
                context.device_id = None;
                context.distribution_tenant_id = Self::unset("distribution_tenant_id");
            }
            state.metrics.service.update_metrics_context(Some(context));
        }
    }

    pub fn update_session_id(&self, value: Option<String>) {
        let value = value.unwrap_or_default();
        {
            let mut context = self.service.get_context();
            context.device_session_id = value;
            self.service.update_metrics_context(Some(context));
        }
    }

    pub fn set_app_session(&self, session: CompletedSessionResponse) {
        let mut apps = self.app_session.write().unwrap();
        apps.insert(
            session.app_id.clone(),
            SiftAppSession {
                app_session_id: session.loaded_session_id,
                app_user_session_id: session.active_session_id,
                app_metrics_version: None,
            },
        );
    }

    pub fn set_app_metrics_version(&self, app_id: &str, version: String) -> Result<(), AppError> {
        let mut apps = self.app_session.write().unwrap();
        if let Some(app) = apps.get_mut(app_id) {
            app.app_metrics_version = Some(version);
            return Ok(());
        }
        Err(AppError::NotFound)
    }
}

pub async fn send_metric(
    platform_state: &DistributorState,
    mut payload: BehavioralMetricPayload,
    ctx: &CallContext,
) -> RippleResponse {
    // TODO use _ctx for any governance stuff
    let drop_data = update_app_context(platform_state, ctx, &mut payload).await;
    /*
    not opted in, or configured out, do nothing
    */
    if drop_data {
        debug!("drop data is true, not sending BI metrics");
        return Ok(());
    }

    if let Some(v) = platform_state
        .metrics
        .service
        .send_behavioral(payload)
        .await
    {
        if v {
            platform_state.metrics.check_send();
        }
    }

    Ok(())
}

pub async fn update_app_context(
    ps: &DistributorState,
    ctx: &CallContext,
    payload: &mut impl IUpdateContext,
) -> bool {
    let mut context: BehavioralMetricContext = ctx.clone().into();
    if let Some(app) = ps.metrics.get_app_session(&ctx.app_id) {
        context.app_session_id = app.app_session_id;
        context.app_user_session_id = app.app_user_session_id;
        context.product_version = ps
            .metrics
            .version
            .clone()
            .unwrap_or(String::from(SEMVER_LIGHTWEIGHT));

        context.app_version = app.app_metrics_version.clone();
    } else {
        error!("AppSession Not Available for metrics {:?}", ctx);
    }
    if let Some(session) = ps.get_account_session() {
        context.partner_id = session.id;
    }
    let (tags, drop_data) =
        DataGovernance::resolve_tags(ps, ctx.app_id.clone(), DataEventType::BusinessIntelligence)
            .await;
    let tag_name_set = tags.iter().map(|tag| tag.tag_name.clone()).collect();
    context.governance_state = Some(AppDataGovernanceState::new(tag_name_set));

    payload.update_context(context);
    drop_data
}
