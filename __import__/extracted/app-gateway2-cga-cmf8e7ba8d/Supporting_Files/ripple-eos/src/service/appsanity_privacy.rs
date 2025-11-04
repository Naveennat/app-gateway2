use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::HashMap;
use std::str::FromStr;
use std::time::{Duration, UNIX_EPOCH};
use url::{ParseError, Url};

use crate::util::http_client::{HttpClient, HttpError};
use thunder_ripple_sdk::ripple_sdk::{
    api::{
        device::device_user_grants_data::{GrantLifespan, GrantStatus},
        distributor::distributor_privacy::{
            DataEventType, ExclusionPolicy, ExclusionPolicyData, GetPropertyParams,
            PrivacyResponse, PrivacySetting, PrivacySettings, SetPropertyParams,
        },
        distributor::distributor_usergrants::UserGrantsCloudSetParams,
        firebolt::fb_capabilities::CapabilityRole,
        session::AccountSession,
        usergrant_entry::UserGrantInfo,
    },
    log::{debug, error, warn},
};

const ENTITY_REFERENCE_PREFIX: &'static str = "xrn:xvp:application:";
const OWNER_REFERENCE_PREFIX: &'static str = "xrn:xcal:subscriber:account:";

#[derive(Debug, Serialize, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct EssSettingData {
    pub allowed: Option<bool>,
    pub expiration: Option<String>,
    pub owner_reference: Option<String>,
    pub entity_reference: Option<String>,
    #[serde(skip_serializing)]
    pub updated: Option<String>,
}

#[derive(Debug, Deserialize)]
pub enum ValueType {
    Many(Vec<EssSettingData>),
    Single(EssSettingData),
}
impl TryFrom<Value> for ValueType {
    type Error = &'static str;

    fn try_from(value: Value) -> Result<Self, Self::Error> {
        if value.is_array() {
            let mut list: Vec<EssSettingData> = Vec::new();
            for v in value.as_array().unwrap() {
                debug!("About to convert: {:?}", v);
                let setting = serde_json::from_value::<EssSettingData>(v.clone()).map_err(|d| {
                    debug!("error: {:?}", d);
                    "Unable to convert value to ess settings struct"
                });
                //TODO: Have to decide on what to be done if we cant unwrap safely.
                list.push(setting.unwrap());
            }
            return Ok(Self::Many(list));
        } else if value.is_object() {
            let setting = serde_json::from_value::<EssSettingData>(value)
                .map_err(|_e| "Unable to convert value to single ess struct")?;
            return Ok(Self::Single(setting));
        } else {
            return Err("Unable to convert");
        }
    }
}

impl EssSettingData {
    pub fn new(
        allowed: Option<bool>,
        expiration: Option<String>,
        owner_reference: Option<String>,
        entity_reference: Option<String>,
        updated: Option<String>,
    ) -> Self {
        EssSettingData {
            allowed,
            expiration,
            owner_reference,
            entity_reference,
            updated,
        }
    }

    pub fn as_app_id(&self) -> Option<String> {
        match &self.entity_reference {
            Some(reference) => parse_entity_reference(reference),
            None => None,
        }
    }

    pub fn get_timestamp_str_from_duration(duration: Duration) -> String {
        let datetime: DateTime<Utc> = (UNIX_EPOCH + duration).into();
        datetime.format("%Y-%m-%dT%H:%M:%SZ").to_string()
    }
}

pub fn parse_entity_reference(reference: &String) -> Option<String> {
    if reference.starts_with(ENTITY_REFERENCE_PREFIX) {
        match reference.get(ENTITY_REFERENCE_PREFIX.len()..) {
            Some(s) => Some(s.to_string()),
            None => None,
        }
    } else {
        warn!(
            "{} {}",
            reference, "as_app_id: Entity reference does not conform to XVP URN expectations"
        );
        return Some(reference.clone());
    }
}

impl Default for EssSettingData {
    fn default() -> Self {
        EssSettingData {
            allowed: None,
            expiration: None,
            owner_reference: None,
            entity_reference: None,
            updated: None,
        }
    }
}

#[derive(Debug, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct Settings {
    #[serde(rename = "xcal:appDataCollection")]
    app_data_collection: Option<Vec<EssSetting>>,
    #[serde(rename = "xcal:appEntitlementCollection")]
    app_entitlement_collection: Option<Vec<EssSetting>>,
    #[serde(rename = "xcal:businessAnalytics")]
    business_analytics: Option<EssSetting>,
    #[serde(rename = "xcal:continueWatching")]
    continue_watching: Option<EssSetting>,
    #[serde(rename = "xcal:unentitledContinueWatching")]
    unentitled_continue_watching: Option<EssSetting>,
    #[serde(rename = "xcal:watchHistory")]
    watch_history: Option<EssSetting>,
    #[serde(rename = "xcal:productAnalytics")]
    product_analytics: Option<EssSetting>,
    #[serde(rename = "xcal:personalization")]
    personalization: Option<EssSetting>,
    #[serde(rename = "xcal:unentitledPersonalization")]
    unentitled_personalization: Option<EssSetting>,
    #[serde(rename = "xcal:remoteDiagnostics")]
    remote_diagnostics: Option<EssSetting>,
    #[serde(rename = "xcal:primaryContentAdTargeting")]
    primary_content_ad_targeting: Option<EssSetting>,
    #[serde(rename = "xcal:primaryBrowseAdTargeting")]
    primary_browse_ad_targeting: Option<EssSetting>,
    #[serde(rename = "xcal:appContentAdTargeting")]
    app_content_ad_targeting: Option<EssSetting>,
    #[serde(rename = "xcal:acr")]
    acr: Option<EssSetting>,
    #[serde(rename = "xcal:cameraAnalytics")]
    camera_analytics: Option<EssSetting>,
}

#[derive(Debug, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
struct EssGetResponseBody {
    _partner_id: String,
    _account_id: String,
    settings: Settings,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct RawEssGetResponseBody {
    _partner_id: String,
    _account_id: String,
    settings: HashMap<String, Value>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CapRole {
    pub role: CapabilityRole,
    pub cap: String,
}

impl RawEssGetResponseBody {
    fn date_str_to_std_duration_from_epoch(timestamp_str: &str) -> Option<Duration> {
        match timestamp_str.parse::<DateTime<Utc>>() {
            Ok(dt) => {
                if let time_stamp @ 0.. = dt.timestamp() {
                    Some(Duration::from_secs(time_stamp as u64))
                } else {
                    None
                }
            }
            Err(_) => None,
        }
    }
    fn get_cloud_entry(settings: &EssSettingData, cap_role: &CapRole) -> UserGrantInfo {
        let cloud_entry = UserGrantInfo {
            role: cap_role.role.clone(),
            capability: cap_role.cap.to_owned(),
            status: match settings.allowed {
                Some(allowed) => {
                    if allowed {
                        Some(GrantStatus::Allowed)
                    } else {
                        Some(GrantStatus::Denied)
                    }
                }
                _ => Some(GrantStatus::Denied),
            },
            last_modified_time: match settings.updated.as_ref() {
                Some(timestamp_str) => Self::date_str_to_std_duration_from_epoch(timestamp_str)
                    .unwrap_or(Duration::new(0, 0)),
                None => Duration::new(0, 0),
            },
            app_name: settings
                .entity_reference
                .as_ref()
                .map(|reference| parse_entity_reference(reference).unwrap_or(reference.to_owned())),
            expiry_time: settings.expiration.as_ref().map(|timestamp_str| {
                Self::date_str_to_std_duration_from_epoch(timestamp_str)
                    .unwrap_or(Duration::new(0, 0))
            }),
            lifespan: match settings.expiration {
                Some(_) => GrantLifespan::Seconds,
                None => GrantLifespan::Forever,
            },
        };
        cloud_entry
    }

    pub fn get_grants(&self, user_grants_mapping: &HashMap<String, CapRole>) -> Vec<UserGrantInfo> {
        let mut cloud_grant_entries: Vec<UserGrantInfo> = Vec::new();
        let ess_settings = &self.settings;
        for (k, v) in ess_settings {
            debug!("key: {k}");
            let cap_role_opt = user_grants_mapping.get(k);
            if cap_role_opt.is_none() {
                continue;
            }
            let cap_role = cap_role_opt.unwrap();
            let vt: Result<ValueType, &str> = v.clone().try_into();
            debug!("{k}: {:?}\n \n", vt);
            if let Ok(v) = vt {
                match v {
                    ValueType::Single(entry) => {
                        cloud_grant_entries.push(Self::get_cloud_entry(&entry, &cap_role))
                    }
                    ValueType::Many(entries) => {
                        for entry in entries {
                            cloud_grant_entries.push(Self::get_cloud_entry(&entry, &cap_role))
                        }
                    }
                }
            }
        }
        cloud_grant_entries
    }
}
impl EssGetResponseBody {
    pub fn get_setting(&self, setting: PrivacySetting) -> Option<EssSetting> {
        fn get_app_setting(
            app_id: String,
            settings: Option<Vec<EssSetting>>,
        ) -> Option<EssSetting> {
            if let None = settings {
                return None;
            }
            for setting in settings.unwrap() {
                let id = setting.data.as_app_id();
                if id.is_none() {
                    continue;
                }
                if id.unwrap().eq(&app_id) {
                    return Some(setting);
                }
            }
            None
        }

        match setting {
            PrivacySetting::AppDataCollection(app_id) => {
                get_app_setting(app_id, self.settings.app_data_collection.clone())
            }
            PrivacySetting::AppEntitlementCollection(app_id) => {
                get_app_setting(app_id, self.settings.app_entitlement_collection.clone())
            }
            PrivacySetting::BusinessAnalytics => self.settings.business_analytics.clone(),
            PrivacySetting::ContinueWatching => self.settings.continue_watching.clone(),
            PrivacySetting::UnentitledContinueWatching => {
                self.settings.unentitled_continue_watching.clone()
            }
            PrivacySetting::WatchHistory => self.settings.watch_history.clone(),
            PrivacySetting::ProductAnalytics => self.settings.product_analytics.clone(),
            PrivacySetting::Personalization => self.settings.personalization.clone(),
            PrivacySetting::UnentitledPersonalization => {
                self.settings.unentitled_personalization.clone()
            }
            PrivacySetting::RemoteDiagnostics => self.settings.remote_diagnostics.clone(),
            PrivacySetting::PrimaryContentAdTargeting => {
                self.settings.primary_content_ad_targeting.clone()
            }
            PrivacySetting::PrimaryBrowseAdTargeting => {
                self.settings.primary_browse_ad_targeting.clone()
            }
            PrivacySetting::AppContentAdTargeting => self.settings.app_content_ad_targeting.clone(),
            PrivacySetting::Acr => self.settings.acr.clone(),
            PrivacySetting::CameraAnalytics => self.settings.camera_analytics.clone(),
        }
    }

    pub fn get_settings(&self) -> PrivacySettings {
        fn get_allowed(setting: &Option<EssSetting>) -> Option<bool> {
            match setting {
                Some(s) => s.data.allowed,
                None => None,
            }
        }

        /* //This function is not used anywhere. Uncommnet if needed
        fn get_allowed_apps(settings: &Option<Vec<EssSetting>>) -> Option<Vec<AppSetting>> {
            if let None = settings {
                return None;
            }

            let mut app_settings = Vec::new();

            for setting in settings.clone().unwrap() {
                app_settings.push(AppSetting {
                    app_id: setting.data.as_app_id(),
                    value: setting.data.allowed,
                });
            }

            if app_settings.is_empty() {
                return None;
            }

            Some(app_settings)
        }
        */

        PrivacySettings {
            //app_data_collection: get_allowed_apps(&self.settings.app_data_collection),
            //app_entitlement_collection: get_allowed_apps(&self.settings.app_entitlement_collection),
            allow_business_analytics: get_allowed(&self.settings.business_analytics)
                .unwrap_or(true),
            allow_resume_points: get_allowed(&self.settings.continue_watching).unwrap_or_default(),
            allow_unentitled_resume_points: get_allowed(
                &self.settings.unentitled_continue_watching,
            )
            .unwrap_or_default(),
            allow_watch_history: get_allowed(&self.settings.watch_history).unwrap_or_default(),
            allow_product_analytics: get_allowed(&self.settings.product_analytics)
                .unwrap_or_default(),
            allow_personalization: get_allowed(&self.settings.personalization).unwrap_or_default(),
            allow_unentitled_personalization: get_allowed(
                &self.settings.unentitled_personalization,
            )
            .unwrap_or_default(),
            allow_remote_diagnostics: get_allowed(&self.settings.remote_diagnostics)
                .unwrap_or_default(),
            allow_primary_content_ad_targeting: get_allowed(
                &self.settings.primary_content_ad_targeting,
            )
            .unwrap_or_default(),
            allow_primary_browse_ad_targeting: get_allowed(
                &self.settings.primary_browse_ad_targeting,
            )
            .unwrap_or_default(),
            allow_app_content_ad_targeting: get_allowed(&self.settings.app_content_ad_targeting)
                .unwrap_or_default(),
            allow_acr_collection: get_allowed(&self.settings.acr).unwrap_or_default(),
            allow_camera_analytics: get_allowed(&self.settings.camera_analytics)
                .unwrap_or_default(),
        }
    }
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct EssSetting {
    #[serde(skip)]
    pub name: &'static str,
    #[serde(flatten)]
    pub data: EssSettingData,
}

// XXX: fmt::Display or something fancy
fn setting_to_str(privacy_setting: &PrivacySetting) -> &'static str {
    let name = match privacy_setting {
        PrivacySetting::AppDataCollection(_) => "xcal:appDataCollection",
        PrivacySetting::AppEntitlementCollection(_) => "xcal:appEntitlementCollection",
        PrivacySetting::BusinessAnalytics => "xcal:businessAnalytics",
        PrivacySetting::ContinueWatching => "xcal:continueWatching",
        PrivacySetting::UnentitledContinueWatching => "xcal:unentitledContinueWatching",
        PrivacySetting::WatchHistory => "xcal:watchHistory",
        PrivacySetting::ProductAnalytics => "xcal:productAnalytics",
        PrivacySetting::Personalization => "xcal:personalization",
        PrivacySetting::UnentitledPersonalization => "xcal:unentitledPersonalization",
        PrivacySetting::RemoteDiagnostics => "xcal:remoteDiagnostics",
        PrivacySetting::PrimaryContentAdTargeting => "xcal:primaryContentAdTargeting",
        PrivacySetting::PrimaryBrowseAdTargeting => "xcal:primaryBrowseAdTargeting",
        PrivacySetting::AppContentAdTargeting => "xcal:appContentAdTargeting",
        PrivacySetting::Acr => "xcal:acr",
        PrivacySetting::CameraAnalytics => "xcal:cameraAnalytics",
    };
    name
}

impl EssSetting {
    pub fn new(
        privacy_setting: PrivacySetting,
        value: Option<bool>,
        expiration: Option<String>,
        owner_reference: Option<String>,
        entity_reference: Option<String>,
        updated: Option<String>,
    ) -> EssSetting {
        let data = EssSettingData::new(
            value,
            expiration,
            owner_reference,
            entity_reference,
            updated,
        );
        let name = setting_to_str(&privacy_setting);
        EssSetting { name, data }
    }

    pub fn to_body(&self) -> String {
        format!(
            "{{\"{}\": {}}}",
            self.name,
            serde_json::to_string(&self.data).unwrap()
        )
    }
}

impl Default for EssSetting {
    fn default() -> Self {
        EssSetting {
            name: "",
            data: EssSettingData::default(),
        }
    }
}

#[derive(Debug, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
struct ExclusionData {
    #[serde(rename = "dataEvents")]
    data_events: Vec<String>,
    #[serde(rename = "entityReference")]
    entity_reference: Vec<String>,
    #[serde(rename = "derivativePropagation")]
    derivative_propagation: bool,
}

// XXX: use genrics <T> for struct Settings
#[derive(Debug, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
struct Exclusions<T> {
    #[serde(rename = "xcal:acr")]
    acr: Option<T>,
    #[serde(rename = "xcal:appContentAdTargeting")]
    app_content_ad_targeting: Option<T>,
    #[serde(rename = "xcal:businessAnalytics")]
    business_analytics: Option<ExclusionData>,
    #[serde(rename = "xcal:cameraAnalytics")]
    camera_analytics: Option<T>,
    #[serde(rename = "xcal:continueWatching")]
    continue_watching: Option<T>,
    #[serde(rename = "xcal:personalization")]
    personalization: Option<T>,
    #[serde(rename = "xcal:primaryBrowseAdTargeting")]
    primary_browse_ad_targeting: Option<T>,
    #[serde(rename = "xcal:primaryContentAdTargeting")]
    primary_content_ad_targeting: Option<T>,
    #[serde(rename = "xcal:productAnalytics")]
    product_analytics: Option<T>,
    #[serde(rename = "xcal:remoteDiagnostics")]
    remote_diagnostics: Option<T>,
    #[serde(rename = "xcal:unentitledContinueWatching")]
    unentitled_continue_watching: Option<T>,
    #[serde(rename = "xcal:unentitledPersonalization")]
    unentitled_personalization: Option<T>,
    #[serde(rename = "xcal:watchHistory")]
    watch_history: Option<T>,
}

#[derive(Debug, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
struct ExclusionGetResponseBody {
    //partner_id: String,
    #[serde(rename = "exclusionPolicy")]
    exclusions: Exclusions<ExclusionData>,
}

impl ExclusionGetResponseBody {
    pub fn get_exclusions(&self) -> ExclusionPolicy {
        ExclusionPolicy {
            acr: self.get_data(self.exclusions.acr.clone()),
            app_content_ad_targeting: self
                .get_data(self.exclusions.app_content_ad_targeting.clone()),
            business_analytics: self.get_data(self.exclusions.business_analytics.clone()),
            camera_analytics: self.get_data(self.exclusions.camera_analytics.clone()),
            continue_watching: self.get_data(self.exclusions.continue_watching.clone()),
            personalization: self.get_data(self.exclusions.personalization.clone()),
            primary_browse_ad_targeting: self
                .get_data(self.exclusions.primary_browse_ad_targeting.clone()),
            primary_content_ad_targeting: self
                .get_data(self.exclusions.primary_content_ad_targeting.clone()),
            product_analytics: self.get_data(self.exclusions.product_analytics.clone()),
            remote_diagnostics: self.get_data(self.exclusions.remote_diagnostics.clone()),
            unentitled_continue_watching: self
                .get_data(self.exclusions.unentitled_continue_watching.clone()),
            unentitled_personalization: self
                .get_data(self.exclusions.unentitled_personalization.clone()),
            watch_history: self.get_data(self.exclusions.watch_history.clone()),
        }
    }
    pub fn get_data(&self, excl_data: Option<ExclusionData>) -> Option<ExclusionPolicyData> {
        match excl_data {
            None => None,
            Some(data) => Some(ExclusionPolicyData {
                data_events: get_data_events(data.data_events),
                entity_reference: get_enitity_reference(data.entity_reference),
                derivative_propagation: data.derivative_propagation,
            }),
        }
    }
}

pub fn get_data_events(de_list: Vec<String>) -> Vec<DataEventType> {
    let mut vec = Vec::new();
    for de in de_list {
        debug!("events string: {:?}", de);
        vec.push(DataEventType::from_str(&de).unwrap());
    }
    vec
}

pub fn get_enitity_reference(er_list: Vec<String>) -> Vec<String> {
    let mut vec = Vec::new();
    for er in er_list {
        vec.push(parse_entity_reference(&er).unwrap_or(er));
    }
    vec
}

struct EssUri {
    endpoint: String,
    partner_id: String,
    account_id: String,
    client_id: String,
    setting: Option<String>,
    entity: Option<String>,
    show_expired: Option<bool>,
}

impl EssUri {
    pub fn new(
        endpoint: String,
        partner_id: String,
        account_id: String,
        setting: Option<String>,
        entity: Option<String>,
        show_expired: Option<bool>,
    ) -> Self {
        EssUri {
            endpoint,
            partner_id,
            account_id,
            client_id: "ripple".into(),
            setting,
            entity,
            show_expired,
        }
    }

    pub fn into_privacy_settings_url(&self) -> Result<String, HttpError> {
        let mut url = Url::parse(&self.endpoint)?;
        url.path_segments_mut()
            .map_err(|_| ParseError::SetHostOnCannotBeABaseUrl)?
            .push("v1") // XXX: remove
            .push("partners")
            .push(&self.partner_id)
            .push("accounts")
            .push(&self.account_id)
            .push("privacySettings");

        url.query_pairs_mut()
            .append_pair("clientId", &self.client_id);

        if let Some(setting) = &self.setting {
            url.query_pairs_mut().append_pair("settingFilter", setting);
        }

        if let Some(entity) = &self.entity {
            url.query_pairs_mut()
                .append_pair("entityReferenceFilter", entity);
        }

        if let Some(se) = &self.show_expired {
            let show_expired = match se {
                true => "true",
                false => "false",
            };
            url.query_pairs_mut()
                .append_pair("showExpired", show_expired);
        }

        Ok(String::from(url))
    }

    pub fn into_usage_data_exclusion_url(&self) -> Result<String, HttpError> {
        let mut url = Url::parse(&self.endpoint)?;
        url.path_segments_mut()
            .map_err(|_| ParseError::SetHostOnCannotBeABaseUrl)?
            .push("v1")
            .push("partners")
            .push(&self.partner_id)
            .push("privacySettings")
            .push("policy")
            .push("usageDataExclusions");

        url.query_pairs_mut()
            .append_pair("clientId", &self.client_id);

        if let Some(setting) = &self.setting {
            url.query_pairs_mut().append_pair("settingFilter", setting);
        }

        if let Some(entity) = &self.entity {
            url.query_pairs_mut()
                .append_pair("entityReferenceFilter", entity);
        }

        Ok(url.into())
    }
}

#[derive(Debug, Clone)]
pub struct PrivacyService {
    endpoint: String,
    user_grants_cloud_mapping: HashMap<String, CapRole>,
}

impl PrivacyService {
    pub fn new(endpoint: String, firebolt_cloud_mapping: &Value) -> PrivacyService {
        let user_grants_cloud_mapping = Self::get_user_grants_mapping(firebolt_cloud_mapping);
        PrivacyService {
            endpoint,
            user_grants_cloud_mapping,
        }
    }

    pub fn get_user_grants_mapping(cloud_firebolt_mapping: &Value) -> HashMap<String, CapRole> {
        if !cloud_firebolt_mapping.is_object() {
            // Without mapping information no need to reach for the clouds. Simple return nothing.
            debug!("cloud mapping not present so returning no user grants");
            return HashMap::new();
        }
        let cloud_mapping = cloud_firebolt_mapping.as_object().unwrap();
        if cloud_mapping.get("user_grants").is_none() {
            // Without user grants mapping information also we cant do anything so no need to reach for the clouds.
            // Simply return nothing.
            debug!("cloud mapping present but does not contain user grants mapping so returning empty list");
            return HashMap::new();
        }
        let user_grants_mapping_val = cloud_mapping.get("user_grants").unwrap();
        if !user_grants_mapping_val.is_object() {
            // Seems not a proper config, so returning Nothing
            debug!("cloud mapping and user grants present but user grants is not an object so returning empty list");
            return HashMap::new();
        }
        let user_grants_mapping_res =
            serde_json::from_value::<HashMap<String, CapRole>>(user_grants_mapping_val.clone());
        if let Err(_) = user_grants_mapping_res {
            debug!("could not convert user grants config to useful struct so returning empty list");
            return HashMap::new();
        }
        user_grants_mapping_res.unwrap()
    }

    async fn get_raw_ess_response(
        &self,
        session: &AccountSession,
    ) -> Result<RawEssGetResponseBody, HttpError> {
        let uri = EssUri::new(
            self.endpoint.clone(),
            session.id.clone(),
            session.account_id.clone(),
            None,
            None,
            Some(false),
        );
        let mut http = HttpClient::new();
        let body_string = http
            .set_token(session.token.clone())
            .get(uri.into_privacy_settings_url()?, String::default())
            .await?;
        debug!("Received raw response: {:?}", body_string);
        let resp_body: Result<RawEssGetResponseBody, serde_json::Error> =
            serde_json::from_str(&body_string);
        resp_body.map_err(|e| {
            error!("RawEssGetResponseBody parse error {:?}", e);
            HttpError::ServiceError
        })
    }

    async fn get_ess_response(
        &self,
        session: AccountSession,
    ) -> Result<EssGetResponseBody, HttpError> {
        let mut http = HttpClient::new();
        let uri = EssUri::new(
            self.endpoint.clone(),
            session.id.clone(),
            session.account_id.clone(),
            None,
            None,
            Some(false),
        );
        let body_string = http
            .set_token(session.token.clone())
            .get(uri.into_privacy_settings_url()?, String::default())
            .await?;
        debug!("Received raw response: {:?}", body_string);
        let resp_body: Result<EssGetResponseBody, serde_json::Error> =
            serde_json::from_str(&body_string);
        resp_body.map_err(|e| {
            error!("EssGetResponseBody parse error {:?}", e);
            HttpError::ServiceError
        })
    }

    pub async fn get_property(
        &self,
        params: GetPropertyParams,
    ) -> Result<PrivacyResponse, HttpError> {
        let entity = match params.setting.clone() {
            PrivacySetting::AppDataCollection(app_id) => Some(app_id),
            PrivacySetting::AppEntitlementCollection(app_id) => Some(app_id),
            _ => None,
        };
        let uri = EssUri::new(
            self.endpoint.clone(),
            params.dist_session.id.clone(),
            params.dist_session.account_id.clone(),
            Some(setting_to_str(&params.setting).to_string()),
            entity,
            Some(false),
        );
        let mut http = HttpClient::new();
        let body_string = http
            .set_token(params.dist_session.token.clone())
            .get(uri.into_privacy_settings_url()?, String::default())
            .await?;
        let resp_body: Result<EssGetResponseBody, serde_json::Error> =
            serde_json::from_str(&body_string);
        if let Ok(r) = resp_body {
            if let Some(setting) = r.get_setting(params.setting) {
                return Ok(PrivacyResponse::Bool(setting.data.allowed.unwrap()));
            }
        }

        Err(HttpError::ServiceError)
    }

    pub async fn set_property(
        &self,
        params: SetPropertyParams,
    ) -> Result<PrivacyResponse, HttpError> {
        let entity = match params.setting.clone() {
            PrivacySetting::AppDataCollection(app_id) => {
                Some(format!("{}{}", ENTITY_REFERENCE_PREFIX, app_id))
            }
            PrivacySetting::AppEntitlementCollection(app_id) => {
                Some(format!("{}{}", ENTITY_REFERENCE_PREFIX, app_id))
            }
            _ => None,
        };
        let owner_reference = format!(
            "{}{}",
            OWNER_REFERENCE_PREFIX, params.dist_session.account_id
        );
        let setting = EssSetting::new(
            params.setting.clone(),
            Some(params.value),
            None,
            Some(owner_reference),
            None,
            None,
        );
        let body = setting.to_body();
        let uri = EssUri::new(
            self.endpoint.clone(),
            params.dist_session.id.clone(),
            params.dist_session.account_id.clone(),
            None,
            entity,
            None,
        );

        let mut http = HttpClient::new();
        http.set_token(params.dist_session.token.clone())
            .put(uri.into_privacy_settings_url()?, body)
            .await?;
        Ok(PrivacyResponse::None)
    }

    pub async fn get_properties(
        &self,
        session: AccountSession,
    ) -> Result<PrivacyResponse, HttpError> {
        let resp_body = self.get_ess_response(session).await;
        if let Ok(r) = resp_body {
            return Ok(PrivacyResponse::Settings(r.get_settings()));
        }
        Err(HttpError::ServiceError)
    }

    pub async fn get_partner_exclusions(
        &self,
        session: AccountSession,
    ) -> Result<ExclusionPolicy, HttpError> {
        let uri = EssUri::new(
            self.endpoint.clone(),
            session.id.clone(),
            session.account_id.clone(),
            None,
            None,
            Some(false),
        );
        let mut http = HttpClient::new();
        let response = http
            .set_token(session.token.clone())
            .get(uri.into_usage_data_exclusion_url()?, String::default())
            .await;
        match response {
            Ok(body_string) => {
                let resp_body: Result<ExclusionGetResponseBody, serde_json::Error> =
                    serde_json::from_str(&body_string);
                if let Ok(exclusions_obj) = resp_body {
                    return Ok(exclusions_obj.get_exclusions());
                }
            }
            Err(e) => match e {
                _not_data_found => {
                    return Ok(ExclusionPolicy::default());
                }
            },
        }
        Err(HttpError::ServiceError)
    }

    pub async fn get_user_grants(
        &self,
        account_sesssion: AccountSession,
    ) -> Result<PrivacyResponse, HttpError> {
        let resp_body = self.get_raw_ess_response(&account_sesssion).await;
        debug!("Received ESS response body: {:?}", resp_body);
        if let Ok(resp) = resp_body {
            return Ok(PrivacyResponse::Grants(
                resp.get_grants(&self.user_grants_cloud_mapping),
            ));
        }
        Err(HttpError::ServiceError)
    }

    pub async fn set_user_grant(
        &self,
        params: &UserGrantsCloudSetParams,
    ) -> Result<PrivacyResponse, HttpError> {
        let session = &params.account_session;
        let grant_entry = params.user_grant_info.clone();
        let entity = grant_entry
            .app_name
            .clone()
            .map(|app_name| format!("{}{}", ENTITY_REFERENCE_PREFIX, app_name));

        let owner_reference = format!("{}{}", OWNER_REFERENCE_PREFIX, session.account_id);
        let mut property_name = None;
        for (k, v) in &self.user_grants_cloud_mapping {
            if v.cap.eq(&params.user_grant_info.capability) && v.role == params.user_grant_info.role
            {
                property_name = Some(k.clone());
                break;
            }
        }
        if property_name.is_none() {
            error!(
                "Unable to find property for cap: {} with role: {:?}",
                params.user_grant_info.capability, params.user_grant_info.role
            );
            return Err(HttpError::NotDataFound);
        }
        let ess_settings_data = EssSettingData {
            allowed: Some(match params.user_grant_info.status {
                Some(GrantStatus::Allowed) => true,
                Some(GrantStatus::Denied) => false,
                None => false,
            }),
            expiration: params
                .user_grant_info
                .expiry_time
                .map(|duration| EssSettingData::get_timestamp_str_from_duration(duration)),
            owner_reference: Some(owner_reference),
            entity_reference: entity.clone(),
            updated: None,
        };

        let mut body = "".to_string();
        let mut setting = None;
        if params.user_grant_info.status.is_some() {
            body = format!(
                "{{\"{}\": [{}]}}",
                property_name.unwrap(),
                serde_json::to_string(&ess_settings_data)?
            )
            .to_string();
            debug!("Formed body for user grants: {:?}", body);
        } else {
            setting = property_name;
        }
        let uri = EssUri::new(
            self.endpoint.clone(),
            session.id.clone(),
            session.account_id.clone(),
            setting,
            entity,
            None,
        );
        let url = uri.into_privacy_settings_url()?;

        let mut http = HttpClient::new();
        http.set_token(session.token.clone());
        if params.user_grant_info.status.is_some() {
            http.put(url, body).await?;
        } else {
            http.delete(url, body).await?;
        }
        Ok(PrivacyResponse::None)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use ripple_sdk::api::firebolt::fb_capabilities::CapabilityRole;
    use std::time::SystemTime;

    #[test]
    fn test_get_cloud_entry() {
        let ess_setting = EssSettingData::new(
            Some(true),
            Some("2025-07-16T14:27:32Z".to_string()),
            Some("xrn:xcal:subscriber:account:5910694746181713281".to_string()),
            Some("xrn:xvp:application:comcast.test.firecert".to_string()),
            Some("2024-07-16T14:27:33.153249614Z".to_string()),
        );

        let cap_role = CapRole {
            role: CapabilityRole::Use,
            cap: "com.ess".to_string(),
        };

        let usergrant_info_from_cloud =
            RawEssGetResponseBody::get_cloud_entry(&ess_setting, &cap_role);

        let lifespan_ttl_in_secs = usergrant_info_from_cloud.expiry_time.map(|expiry| {
            expiry
                .as_secs()
                .saturating_sub(usergrant_info_from_cloud.last_modified_time.as_secs())
        });

        let expires = {
            lifespan_ttl_in_secs.map(|ttl_secs| {
                let expiry_system_time: SystemTime = SystemTime::UNIX_EPOCH
                    + usergrant_info_from_cloud.last_modified_time
                    + Duration::from_secs(ttl_secs);
                let expiry_date_time: DateTime<Utc> = DateTime::from(expiry_system_time);
                expiry_date_time.to_rfc3339()
            })
        };

        assert_eq!(
            expires.unwrap(),
            "2025-07-16T14:27:32+00:00".to_string(),
            "Expiry time is not as expected"
        );
    }
}
