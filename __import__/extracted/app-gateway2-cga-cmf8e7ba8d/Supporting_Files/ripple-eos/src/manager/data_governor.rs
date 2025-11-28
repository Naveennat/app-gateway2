use std::collections::HashSet;
use std::sync::{Arc, RwLock};
use thunder_ripple_sdk::ripple_sdk::api::storage_manager::StorageManager;
use thunder_ripple_sdk::ripple_sdk::{
    api::{
        distributor::distributor_privacy::{DataEventType, ExclusionPolicy, ExclusionPolicyData},
        firebolt::fb_discovery::DataTagInfo,
        manifest::device_manifest::DataGovernancePolicy,
        storage_property::StorageProperty,
    },
    log::{debug, info},
    utils::error::RippleError,
};

use crate::state::distributor_state::DistributorState;

pub fn default_enforcement_value() -> bool {
    false
}

pub fn default_drop_on_all_tags() -> bool {
    true
}

pub struct DataGovernance {}

#[derive(Clone)]
pub struct DataGovernanceState {
    pub exclusions: Arc<RwLock<Option<ExclusionPolicy>>>,
}

impl Default for DataGovernanceState {
    fn default() -> Self {
        DataGovernanceState {
            exclusions: Arc::new(RwLock::new(None)),
        }
    }
}

impl std::fmt::Debug for DataGovernanceState {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("DataGovernanceState").finish()
    }
}

impl DataGovernance {
    fn update_local_exclusion_policy(state: &DataGovernanceState, excl: ExclusionPolicy) {
        let mut dg = state.exclusions.write().unwrap();
        *dg = Some(excl)
    }

    fn get_local_exclusion_policy(state: &DataGovernanceState) -> Option<ExclusionPolicy> {
        let dg = state.exclusions.read().unwrap();
        (*dg).clone()
    }

    pub async fn get_tags(
        state: &DistributorState,
        app_id: String,
        data_type: DataEventType,
        policy: &DataGovernancePolicy,
    ) -> (HashSet<DataTagInfo>, bool) {
        let mut tags = HashSet::default();
        let mut all_settings_enforced = true;
        let exclusions = DataGovernance::get_partner_exclusions(state)
            .await
            .unwrap_or_default();
        for tag in &policy.setting_tags {
            let mut excluded = false;
            let mut propagation_state = true;
            let data = DataGovernance::get_exclusion_data(tag.setting.clone(), exclusions.clone());
            if let Some(d) = data {
                let (excluded_tmp, propagation_state_tmp) =
                    DataGovernance::is_app_excluded_and_get_propagation_state(
                        &app_id, &data_type, &d,
                    );
                excluded = excluded_tmp;
                propagation_state = propagation_state_tmp;
                debug!(
                    "get_tags: app_id={:?} setting={:?} is_excluded={:?}",
                    app_id.clone(),
                    tag,
                    excluded
                );
            }

            // do not get user setting if excluded
            if excluded {
                let tags_to_add: HashSet<DataTagInfo> = tag
                    .tags
                    .iter()
                    .cloned()
                    .map(|name| DataTagInfo {
                        tag_name: name,
                        propagation_state,
                    })
                    .collect();
                tags.extend(tags_to_add);
            } else {
                let val = StorageManager::get_bool(state, tag.setting.clone())
                    .await
                    .unwrap_or(false);
                if val == tag.enforcement_value {
                    let tags_to_add: HashSet<DataTagInfo> = tag
                        .tags
                        .iter()
                        .cloned()
                        .map(|name| DataTagInfo {
                            tag_name: name,
                            propagation_state: true,
                        })
                        .collect();
                    tags.extend(tags_to_add);
                } else {
                    all_settings_enforced = false;
                }
            }
        }
        (tags, all_settings_enforced)
    }

    pub async fn resolve_tags(
        platform_state: &DistributorState,
        app_id: String,
        data_type: DataEventType,
    ) -> (HashSet<DataTagInfo>, bool) {
        let data_gov_cfg = platform_state
            .device_manifest
            .configuration
            .data_governance
            .clone();
        let data_tags = match data_gov_cfg.get_policy(data_type.clone()) {
            Some(policy) => {
                let (t, all) =
                    DataGovernance::get_tags(platform_state, app_id, data_type, &policy).await;
                if policy.drop_on_all_tags && all {
                    return (t, true);
                }
                t
            }
            None => {
                info!("data_governance.policies not found");
                HashSet::default()
            }
        };
        (data_tags, false)
    }

    pub async fn refresh_partner_exclusions(state: &DistributorState) {
        if let Ok(excl) = Self::get_exclusions_from_privacy_service(state).await {
            DataGovernance::update_local_exclusion_policy(&state.data_governance, excl.clone());
            let result = serde_json::to_string(&excl);
            // result.unwrap_or("");    // XXX: when server return 404 or empty string
            if let Ok(res) = result {
                info!("Persisting exclusions {}", res);
                let _ = StorageManager::set_string(
                    state,
                    StorageProperty::PartnerExclusions,
                    res,
                    None,
                )
                .await;
            }
        }
    }

    async fn get_exclusions_from_privacy_service(
        state: &DistributorState,
    ) -> Result<ExclusionPolicy, RippleError> {
        if let Some(session) = state.get_account_session() {
            return if let Ok(v) = state.privacy_service.get_partner_exclusions(session).await {
                Ok(v)
            } else {
                Err(RippleError::ServiceError)
            };
        }

        Err(RippleError::ServiceError)
    }

    pub async fn get_partner_exclusions(
        state: &DistributorState,
    ) -> Result<ExclusionPolicy, RippleError> {
        let result = Err(RippleError::InvalidOutput);
        if let Some(excl) = DataGovernance::get_local_exclusion_policy(&state.data_governance) {
            return Ok(excl);
        }
        result
    }

    pub fn get_exclusion_data(
        setting: StorageProperty,
        exclusions: ExclusionPolicy,
    ) -> Option<ExclusionPolicyData> {
        match setting {
            StorageProperty::AllowPersonalization => exclusions.personalization,
            StorageProperty::AllowProductAnalytics => exclusions.product_analytics,
            StorageProperty::AllowBusinessAnalytics => exclusions.business_analytics,
            _ => None,
        }
    }

    fn is_app_excluded_and_get_propagation_state(
        app_id: &String,
        data_type: &DataEventType,
        excl: &ExclusionPolicyData,
    ) -> (bool, bool) {
        let mut app_found: bool = false;
        let mut event_found: bool = false;
        let mut propagation_state: bool = true;

        for evt in &excl.data_events {
            if *evt == *data_type {
                event_found = true;
                propagation_state = excl.derivative_propagation;
                break;
            }
        }
        if event_found {
            for app in &excl.entity_reference {
                if app.as_str() == app_id {
                    app_found = true;
                    break;
                }
            }
        }
        (app_found, propagation_state)
    }
}
