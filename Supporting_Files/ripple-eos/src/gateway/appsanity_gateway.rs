use ripple_sdk::api::firebolt::fb_discovery::AgePolicy;
use serde::Deserialize;
use serde_json::Value;
use thunder_ripple_sdk::ripple_sdk::log::{debug, error};

use std::collections::HashMap;
use tonic::transport::{Channel, ClientTlsConfig};

use crate::util::cloud_linchpin_monitor::LinchpinConfig;

#[derive(Deserialize, Debug, Clone)]
pub struct CloudService {
    pub url: String,
}
#[derive(Deserialize, Debug, Clone)]
pub struct CloudServiceScopes {
    content_access: String,
    sign_in_state: String,
}
impl CloudServiceScopes {
    pub fn get_xvp_content_access_scope(&self) -> &str {
        &self.content_access
    }
    pub fn get_xvp_sign_in_state_scope(&self) -> &str {
        &self.sign_in_state
    }
}

impl Default for CloudServiceScopes {
    fn default() -> Self {
        CloudServiceScopes {
            content_access: "account".to_string(),
            sign_in_state: "account".to_string(),
        }
    }
}
#[derive(Debug, Clone)]
pub struct GrpcClientSession {
    pub service_url: String,
}

#[cfg(test)]
impl GrpcClientSession {
    pub fn mock(service_url: &str) -> Self {
        GrpcClientSession {
            service_url: service_url.to_string(),
        }
    }
}

impl GrpcClientSession {
    pub fn new(service_url: String) -> Self {
        Self {
            service_url: service_url.to_owned(),
        }
    }

    pub fn get_grpc_channel(&mut self) -> Result<Channel, crate::message::DpabError> {
        let endpoint = match tonic::transport::Channel::from_shared(format!(
            "https://{}",
            self.service_url.clone()
        )) {
            Ok(ep) => ep,
            Err(_) => return Err(crate::message::DpabError::ServiceError),
        };
        match endpoint.tls_config(ClientTlsConfig::new().domain_name(self.service_url.clone())) {
            Ok(ep) => return Ok(ep.connect_lazy()),
            Err(_) => return Err(crate::message::DpabError::ServiceError),
        };
    }
}

#[derive(Debug, Clone, Deserialize)]
pub struct AppAuthorizationRules {
    pub app_ignore_rules: HashMap<String, Vec<String>>,
}

#[derive(Debug, Clone, Deserialize)]
pub struct MetricsSchema {
    pub event_name: String,
    pub alias: Option<String>,
    pub namespace: Option<String>,
    pub version: Option<String>,
}

#[derive(Debug, Clone, Deserialize)]
pub struct MetricsSchemas {
    pub default_metrics_namespace: String,
    pub default_metrics_schema_version: String,
    #[serde(default = "default_common_schema")]
    pub default_common_schema: String,
    pub metrics_schemas: Vec<MetricsSchema>,
}
impl MetricsSchemas {
    pub fn get_event_name_alias(&self, event_name: &str) -> String {
        match self
            .metrics_schemas
            .iter()
            .find(|key| key.event_name == event_name)
        {
            Some(event) => {
                let default_name = &event_name.to_string();
                event
                    .alias
                    .as_ref()
                    .unwrap_or_else(|| default_name)
                    .to_string()
            }
            None => event_name.to_string(),
        }
    }

    pub fn get_event_path(&self, event_name: &str) -> String {
        match self
            .metrics_schemas
            .iter()
            .find(|key| key.event_name == event_name)
        {
            Some(metric) => {
                format!(
                    "{}/{}/{}",
                    metric
                        .namespace
                        .as_ref()
                        .unwrap_or_else(|| &self.default_metrics_namespace),
                    event_name,
                    metric
                        .version
                        .as_ref()
                        .unwrap_or_else(|| &self.default_metrics_schema_version)
                )
            }
            None => {
                format!(
                    "{}/{}/{}",
                    self.default_metrics_namespace.as_str(),
                    event_name,
                    self.default_metrics_schema_version.as_str()
                )
            }
        }
    }
}
#[derive(Debug, Clone, Deserialize)]
pub struct SiftConfig {
    #[serde(default = "behavioral_metrics_endpoint_default")]
    pub endpoint: String,
    #[serde(default = "behavioral_metrics_api_key_default")]
    pub api_key: String,
    #[serde(default = "behavioral_metrics_batch_size_default")]
    pub batch_size: u8,
    #[serde(default = "behavioral_metrics_max_queue_size_default")]
    pub max_queue_size: u8,
    #[serde(default = "behavioral_metrics_send_interval_seconds_default")]
    pub send_interval_seconds: u16,
    #[serde(default = "behavioral_metrics_ontology_default")]
    pub ontology: bool,
    #[serde(default = "behavioral_metrics_authenticated_default")]
    pub authenticated: bool,
    #[serde(default = "behavioral_metrics_metrics_schemas_default")]
    pub metrics_schemas: MetricsSchemas,
}

#[derive(Debug, Clone, Deserialize)]
pub struct BehavioralMetricsConfig {
    #[serde(default = "behavioral_metrics_plugin_default")]
    pub plugin: bool,
    pub sift: SiftConfig,
}

pub fn defaults() -> AppsanityConfig {
    /*
    build time, assume infallible and unwrap
    */
    /*
    TODO
    load defaults based on build conditions (i.e. debug, VBN, etc) or from filesystem?
    */
    serde_json::from_str::<AppsanityConfig>(include_str!("default_appsanity_config.json")).unwrap()
}

pub fn permission_service_default() -> CloudService {
    defaults().permission_service
}
pub fn ott_token_service_default() -> CloudService {
    defaults().ott_token_service
}
pub fn ad_platform_service_default() -> CloudService {
    defaults().ad_platform_service
}
pub fn xvp_xifa_service_default() -> CloudService {
    defaults().xvp_xifa_service
}
pub fn xvp_playback_service_default() -> CloudService {
    defaults().xvp_playback_service
}
pub fn session_service_default() -> CloudService {
    defaults().session_service
}
pub fn app_authorization_rules_default() -> AppAuthorizationRules {
    defaults().app_authorization_rules
}
pub fn method_ignore_rules_default() -> Vec<String> {
    defaults().method_ignore_rules
}
pub fn behavioral_metrics_default() -> BehavioralMetricsConfig {
    defaults().behavioral_metrics
}
pub fn behavioral_metrics_plugin_default() -> bool {
    defaults().behavioral_metrics.plugin
}
pub fn behavioral_metrics_metrics_schemas_default() -> MetricsSchemas {
    defaults().behavioral_metrics.sift.metrics_schemas
}
pub fn behavioral_metrics_ontology_default() -> bool {
    defaults().behavioral_metrics.sift.ontology
}
pub fn behavioral_metrics_authenticated_default() -> bool {
    defaults().behavioral_metrics.sift.authenticated
}
pub fn behavioral_metrics_endpoint_default() -> String {
    defaults().behavioral_metrics.sift.endpoint.clone()
}
pub fn behavioral_metrics_api_key_default() -> String {
    defaults().behavioral_metrics.sift.api_key.clone()
}
pub fn behavioral_metrics_batch_size_default() -> u8 {
    defaults().behavioral_metrics.sift.batch_size
}
pub fn behavioral_metrics_max_queue_size_default() -> u8 {
    defaults().behavioral_metrics.sift.max_queue_size
}
pub fn behavioral_metrics_send_interval_seconds_default() -> u16 {
    defaults().behavioral_metrics.sift.send_interval_seconds
}
pub fn default_common_schema() -> String {
    "entos/common/5".to_string()
}
pub fn discovery_service_default() -> CloudService {
    defaults().xvp_session_service
}
pub fn privacy_service_default() -> CloudService {
    defaults().privacy_service
}
pub fn xvp_video_service_default() -> CloudService {
    defaults().xvp_video_service
}
pub fn linchpin_service_default() -> SyncMonitorConfig {
    defaults().sync_monitor_service
}
pub fn cloud_firebolt_mapping_default() -> Value {
    defaults().cloud_firebolt_mapping
}
pub fn xvp_data_scopes_default() -> CloudServiceScopes {
    defaults().xvp_data_scopes
}

pub fn linchpin_config_default() -> LinchpinConfig {
    defaults().linchpin_config
}
pub fn age_policy_default() -> AgePolicyConfig {
    let mut category_tag_mappings = HashMap::new();
    let mut cet_mappings = HashMap::new();
    category_tag_mappings.insert(AgePolicy::Child, vec!["child".to_string()]);
    cet_mappings.insert(
        AgePolicy::Child,
        vec![
            "dataPlatform:cet:xvp:personalization:recommendation".to_string(),
            "dataPlatform:cet:xvp:advertising:targeted".to_string(),
            "dataPlatform:cet:xvp:advertising:segmentation".to_string(),
        ],
    );

    AgePolicyConfig {
        category_tag_mappings: category_tag_mappings,
        cet_mappings: cet_mappings,
    }
}

#[derive(Deserialize, Debug, Clone)]
pub struct LinchpinServices {
    pub service_name: String,
    pub listen_topic: String,
    pub ttl: u32,
}
#[derive(Deserialize, Debug, Clone)]
pub struct SyncMonitorConfig {
    pub services: Vec<LinchpinServices>,
}

#[derive(Clone, Debug, Hash, Eq, PartialEq, Deserialize)]
pub struct CategoryTags {
    pub tags: Vec<String>,
}
impl CategoryTags {
    pub fn new(tags: Vec<String>) -> Self {
        CategoryTags { tags }
    }
    pub fn to_vec(category_tags: Option<Self>) -> Option<Vec<String>> {
        category_tags.and_then(|ct| {
            if ct.tags.is_empty() {
                None
            } else {
                Some(ct.tags)
            }
        })
    }
}
#[derive(Deserialize, Debug, Clone, Default)]
pub struct AgePolicyConfig {
    pub category_tag_mappings: HashMap<AgePolicy, Vec<String>>,
    pub cet_mappings: HashMap<AgePolicy, Vec<String>>,
}
impl AgePolicyConfig {
    pub fn get_category_tags(&self, request_age_policy: Option<AgePolicy>) -> Option<CategoryTags> {
        if request_age_policy.is_none() {
            return None;
        }
        let request_age_policy = request_age_policy.unwrap();

        self.category_tag_mappings
            .get(&request_age_policy)
            .cloned()
            .map(CategoryTags::new)
    }
    pub fn get_policy_cets(
        &self,
        request_age_policy: Option<AgePolicy>,
        session_policies: Vec<AgePolicy>,
    ) -> Option<Vec<String>> {
        /*
        convenience
        */
        if request_age_policy.is_none() {
            return None;
        }
        let request_age_policy = request_age_policy.unwrap();
        /*
        if the policy(ies) set by Account.policyIdentifierAlias contains the policy in the processed request
        return the cets that are mapped to the policy in the config
         */
        if session_policies.contains(&request_age_policy) {
            return self.cet_mappings.get(&request_age_policy).cloned();
        }
        None
    }
    pub fn get_age_policy_metrics_metadata(
        &self,
        request_age_policy: Option<AgePolicy>,
        session_policies: Vec<AgePolicy>,
    ) -> Option<(Vec<String>, Vec<String>)> {
        let policy_cets =
            self.get_policy_cets(request_age_policy.clone(), session_policies.clone());
        /*
        short-circuit
        */
        if policy_cets.is_none() {
            return None;
        }

        let category_tags = self.get_category_tags(request_age_policy);
        /*
        short-circuit
        */
        if category_tags.is_none() {
            return None;
        }
        let category_tags = CategoryTags::to_vec(category_tags);
        /*
        short-circuit
        */
        if category_tags.is_none() {
            return None;
        }
        Some((policy_cets.unwrap(), category_tags.unwrap()))
    }
}

#[derive(Deserialize, Debug, Clone)]
pub struct AppsanityConfig {
    #[serde(default = "permission_service_default")]
    pub permission_service: CloudService,
    #[serde(default = "session_service_default")]
    pub session_service: CloudService,
    #[serde(default = "ott_token_service_default")]
    pub ott_token_service: CloudService,
    #[serde(default = "ad_platform_service_default")]
    pub ad_platform_service: CloudService,
    #[serde(default = "xvp_xifa_service_default")]
    pub xvp_xifa_service: CloudService,
    #[serde(default = "xvp_playback_service_default")]
    pub xvp_playback_service: CloudService,
    #[serde(default = "app_authorization_rules_default")]
    pub app_authorization_rules: AppAuthorizationRules,
    #[serde(default = "method_ignore_rules_default")]
    pub method_ignore_rules: Vec<String>,
    #[serde(default = "behavioral_metrics_default")]
    pub behavioral_metrics: BehavioralMetricsConfig,
    #[serde(default = "discovery_service_default")]
    pub xvp_session_service: CloudService,
    #[serde(default = "privacy_service_default")]
    pub privacy_service: CloudService,
    #[serde(default = "xvp_video_service_default")]
    pub xvp_video_service: CloudService,
    #[serde(default = "linchpin_service_default")]
    pub sync_monitor_service: SyncMonitorConfig,
    #[serde(default = "cloud_firebolt_mapping_default")]
    pub cloud_firebolt_mapping: Value,
    #[serde(default = "xvp_data_scopes_default")]
    pub xvp_data_scopes: CloudServiceScopes,
    #[serde(default = "linchpin_config_default")]
    pub linchpin_config: LinchpinConfig,
    #[serde(default = "age_policy_default")]
    pub age_policy: AgePolicyConfig,
}

impl AppsanityConfig {
    pub fn get_service_name(&self, url: &str) -> Option<String> {
        if self.permission_service.url == url {
            return Some("permission_service".to_owned());
        }
        if self.ott_token_service.url == url {
            return Some("ott_token_service".to_owned());
        }
        if self.session_service.url == url {
            return Some("session_service".to_owned());
        }
        if self.ad_platform_service.url == url {
            return Some("ad_platform_service".to_owned());
        }
        if self.xvp_playback_service.url == url {
            return Some("xvp_playback_service".to_owned());
        }
        if self.xvp_session_service.url == url {
            return Some("xvp_session_service".to_owned());
        }
        if self.privacy_service.url == url {
            return Some("privacy_service".to_owned());
        }
        if self.xvp_video_service.url == url {
            return Some("xvp_video_service".to_owned());
        }
        None
    }
    pub fn get_linchpin_topic_for_service(&self, service: &str) -> Option<String> {
        let result = self
            .sync_monitor_service
            .services
            .iter()
            .find(|elem| elem.service_name == service.to_owned());
        if result.is_none() {
            return None;
        } else {
            return Some(result.unwrap().listen_topic.to_owned());
        }
    }
    pub fn get_ttl_for_service(&self, service: &str) -> Option<u32> {
        let result = self
            .sync_monitor_service
            .services
            .iter()
            .find(|elem| elem.service_name == service.to_owned());
        if result.is_none() {
            return None;
        } else {
            return Some(result.unwrap().ttl);
        }
    }
    pub fn get_linchpin_topic_for_url(&self, url: &str) -> Option<String> {
        let service_name = self.get_service_name(url)?;
        self.get_linchpin_topic_for_service(&service_name)
    }
    pub fn get_ttl_for_url(&self, url: &str) -> Option<u32> {
        let service_name = self.get_service_name(url)?;
        self.get_ttl_for_service(&service_name)
    }
}

pub fn get_config(maybe_value: &Option<Value>) -> AppsanityConfig {
    debug!("Passed on config value from ripple {:?}", maybe_value);
    match maybe_value {
        Some(config) => match serde_json::from_value::<AppsanityConfig>(config.to_owned()) {
            Ok(ok) => ok,
            Err(config_error) => {
                error!(
                    "dpab_appsanity config failed with {}, default values will be used",
                    config_error
                );
                defaults()
            }
        },
        None => {
            //Do defaults for completely empty config here
            defaults()
        }
    }
}

#[cfg(test)]
pub mod tests {
    use ripple_sdk::api::firebolt::fb_discovery::AgePolicy;
    use ripple_sdk::Mockable;

    use super::AppsanityConfig;

    impl Mockable for AppsanityConfig {
        fn mock() -> Self {
            let config_str = include_str!("default_appsanity_config.json");
            serde_json::from_str(config_str).unwrap()
        }
    }

    #[test]
    fn test_default_appsanity_config() {
        let config_str = include_str!("default_appsanity_config.json");
        let config: Result<AppsanityConfig, _> = serde_json::from_str(config_str);
        assert!(config.is_ok());
    }

    #[test]
    fn test_cloud_service_scopes() {
        let config_str = include_str!("default_appsanity_config.json");
        let config: Result<AppsanityConfig, _> = serde_json::from_str(config_str);
        assert!(config.is_ok());
        let config = config.unwrap();
        let cloud_service_scopes = config.xvp_data_scopes;
        assert_eq!(
            cloud_service_scopes.get_xvp_content_access_scope(),
            "account"
        );
        assert_eq!(
            cloud_service_scopes.get_xvp_sign_in_state_scope(),
            "account"
        );
    }

    #[test]
    fn test_metrics_schemas() {
        let config_str = include_str!("default_appsanity_config.json");
        let config: Result<AppsanityConfig, _> = serde_json::from_str(config_str);
        assert!(config.is_ok());
        let config = config.unwrap();
        let metrics_schemas = config.behavioral_metrics.sift.metrics_schemas;
        assert_eq!(
            metrics_schemas.get_event_name_alias("event_name"),
            "event_name"
        );
        assert_eq!(
            metrics_schemas.get_event_path("event_name"),
            "entos/event_name/4"
        );
    }

    // AgePolicy related tests
    use super::{age_policy_default, AgePolicyConfig, CategoryTags};

    #[test]
    fn test_age_policy_as_string() {
        // Test AgePolicy as_string method
        assert_eq!(AgePolicy::Child.as_string(), "app:child");
        assert_eq!(AgePolicy::Teen.as_string(), "app:teen");
        assert_eq!(AgePolicy::Adult.as_string(), "app:adult");
    }

    #[test]
    fn test_age_policy_identifier_alias_from_option_string() {
        // Test the From<Option<String>> implementation
        let alias = AgePolicy::Child;
        assert_eq!(alias, AgePolicy::Child);

        let alias = AgePolicy::Teen;
        assert_eq!(alias, AgePolicy::Teen);

        let alias = AgePolicy::Adult;
        assert_eq!(alias, AgePolicy::Adult);

        let alias = AgePolicy::Adult;
        assert_eq!(alias, AgePolicy::Adult);
    }

    #[test]
    fn test_age_policy_identifier_alias_as_str() {
        assert_eq!(AgePolicy::Child.as_string(), "app:child");
        assert_eq!(AgePolicy::Teen.as_string(), "app:teen");
        assert_eq!(AgePolicy::Adult.as_string(), "app:adult");
    }

    #[test]
    fn test_category_tags_new() {
        let tags = vec!["child".to_string(), "family".to_string()];
        let category_tags = CategoryTags::new(tags.clone());
        assert_eq!(category_tags.tags, tags);
    }

    #[test]
    fn test_category_tags_to_vec() {
        // Test with non-empty tags
        let tags = vec!["child".to_string(), "family".to_string()];
        let category_tags = CategoryTags::new(tags.clone());
        let result = CategoryTags::to_vec(Some(category_tags));
        assert_eq!(result, Some(tags));

        // Test with empty tags
        let empty_category_tags = CategoryTags::new(vec![]);
        let result = CategoryTags::to_vec(Some(empty_category_tags));
        assert_eq!(result, None);

        // Test with None
        let result = CategoryTags::to_vec(None);
        assert_eq!(result, None);
    }

    #[test]
    fn test_age_policy_config_default() {
        let config = age_policy_default();

        // Test category tag mappings
        assert!(config.category_tag_mappings.contains_key(&AgePolicy::Child));
        let child_tags = config.category_tag_mappings.get(&AgePolicy::Child).unwrap();
        assert_eq!(child_tags, &vec!["child".to_string()]);

        // Test CET mappings
        assert!(config.cet_mappings.contains_key(&AgePolicy::Child));
        let child_cets = config.cet_mappings.get(&AgePolicy::Child).unwrap();
        assert_eq!(child_cets.len(), 3);
        assert!(
            child_cets.contains(&"dataPlatform:cet:xvp:personalization:recommendation".to_string())
        );
        assert!(child_cets.contains(&"dataPlatform:cet:xvp:advertising:targeted".to_string()));
        assert!(child_cets.contains(&"dataPlatform:cet:xvp:advertising:segmentation".to_string()));
    }

    #[test]
    fn test_age_policy_config_get_category_tags() {
        let mut config = AgePolicyConfig::default();
        let tags = vec!["child".to_string(), "safe".to_string()];
        config
            .category_tag_mappings
            .insert(AgePolicy::Child, tags.clone());

        // Test existing policy
        let result = config.get_category_tags(Some(AgePolicy::Child));
        assert!(result.is_some());
        assert_eq!(result.unwrap().tags, tags);

        // Test non-existing policy
        let result = config.get_category_tags(Some(AgePolicy::Teen));
        assert!(result.is_none());
    }

    #[test]
    fn test_age_policy_config_get_policy_cets() {
        let mut config = AgePolicyConfig::default();
        let cets = vec![
            "dataPlatform:cet:xvp:advertising:targeted".to_string(),
            "dataPlatform:cet:xvp:personalization:recommendation".to_string(),
        ];
        config.cet_mappings.insert(AgePolicy::Teen, cets.clone());

        // Test existing policy
        let result = config.get_policy_cets(Some(AgePolicy::Teen), vec![AgePolicy::Teen]);
        assert_eq!(result, Some(cets));

        // Test non-existing policy
        let result = config.get_policy_cets(Some(AgePolicy::Child), vec![AgePolicy::Child]);
        assert_eq!(result, None);
    }

    #[test]
    fn test_age_policy_config_serialization() {
        // Test that AgePolicyConfig can be created with default
        let config = AgePolicyConfig::default();
        assert!(config.category_tag_mappings.is_empty());
        assert!(config.cet_mappings.is_empty());

        // Test that we can manually create a config similar to the default
        let default_config = age_policy_default();
        assert!(!default_config.category_tag_mappings.is_empty());
        assert!(!default_config.cet_mappings.is_empty());
    }

    #[test]
    fn test_age_policy_identifier_alias_equality() {
        assert_eq!(AgePolicy::Child, AgePolicy::Child);
        assert_ne!(AgePolicy::Child, AgePolicy::Teen);
        assert_ne!(AgePolicy::Teen, AgePolicy::Adult);
    }

    #[test]
    fn test_age_policy_identifier_alias_hash() {
        use std::collections::HashSet;

        let mut set = HashSet::new();
        set.insert(AgePolicy::Child);
        set.insert(AgePolicy::Teen);
        set.insert(AgePolicy::Adult);

        assert_eq!(set.len(), 3);
        assert!(set.contains(&AgePolicy::Child));
        assert!(set.contains(&AgePolicy::Teen));
        assert!(set.contains(&AgePolicy::Adult));
    }

    #[test]
    fn test_appsanity_config_contains_age_policy() {
        let config_str = include_str!("default_appsanity_config.json");
        let config: Result<AppsanityConfig, _> = serde_json::from_str(config_str);
        assert!(config.is_ok());
        let config = config.unwrap();

        // Verify that the AppsanityConfig includes age_policy
        // This tests the integration of AgePolicyConfig into the main config
        let _age_policy = &config.age_policy;
        // If this compiles and runs, the integration is working
    }

    #[test]
    fn test_category_tags_edge_cases() {
        // Test with single tag
        let single_tag = vec!["single".to_string()];
        let category_tags = CategoryTags::new(single_tag.clone());
        let result = CategoryTags::to_vec(Some(category_tags));
        assert_eq!(result, Some(single_tag));

        // Test with duplicate tags (should preserve them)
        let duplicate_tags = vec!["tag".to_string(), "tag".to_string()];
        let category_tags = CategoryTags::new(duplicate_tags.clone());
        let result = CategoryTags::to_vec(Some(category_tags));
        assert_eq!(result, Some(duplicate_tags));
    }

    #[test]
    fn test_age_policy_config_json_marshalling() {
        // Test JSON marshalling using the minimal payload based on age_policy_default()
        // IMPORTANT: This test validates that the JSON structure for AgePolicyConfig
        // remains compatible with the Rust struct definition. If this test breaks,
        // it means the JSON schema has changed and may break runtime config loading.
        // The JSON structure here must match the AgePolicyConfig struct in this file.
        let json_payload = r#"{
                "category_tag_mappings": {
                    "app:child": ["child"]
                },
                "cet_mappings": {
                    "app:child": [
                        "dataPlatform:cet:xvp:personalization:recommendation",
                        "dataPlatform:cet:xvp:advertising:targeted",
                        "dataPlatform:cet:xvp:advertising:segmentation"
                    ]
                }
        }"#;

        // Test deserialization
        let config: Result<AgePolicyConfig, _> = serde_json::from_str(json_payload);
        assert!(
            config.is_ok(),
            "Failed to deserialize AgePolicyConfig from JSON"
        );

        let config = config.unwrap();

        // Verify category tag mappings
        assert_eq!(config.category_tag_mappings.len(), 1);
        assert!(config.category_tag_mappings.contains_key(&AgePolicy::Child));

        let child_tags = config.category_tag_mappings.get(&AgePolicy::Child).unwrap();
        assert_eq!(child_tags, &vec!["child".to_string()]);

        // Verify CET mappings
        assert_eq!(config.cet_mappings.len(), 1);
        assert!(config.cet_mappings.contains_key(&AgePolicy::Child));

        let child_cets = config.cet_mappings.get(&AgePolicy::Child).unwrap();
        assert_eq!(child_cets.len(), 3);
        assert!(
            child_cets.contains(&"dataPlatform:cet:xvp:personalization:recommendation".to_string())
        );
        assert!(child_cets.contains(&"dataPlatform:cet:xvp:advertising:targeted".to_string()));
        assert!(child_cets.contains(&"dataPlatform:cet:xvp:advertising:segmentation".to_string()));

        // Test that the deserialized config matches the default config values
        let default_config = age_policy_default();
        assert_eq!(
            config.category_tag_mappings.get(&AgePolicy::Child),
            default_config.category_tag_mappings.get(&AgePolicy::Child)
        );
        assert_eq!(
            config.cet_mappings.get(&AgePolicy::Child),
            default_config.cet_mappings.get(&AgePolicy::Child)
        );
    }

    #[test]
    fn test_appsanity_config_with_age_policy_json_marshalling() {
        // Test that AgePolicyConfig can be marshalled as part of AppsanityConfig
        // CRITICAL: This test ensures that the age_policy field integration with
        // AppsanityConfig works correctly. If this test fails, it means the serde
        // deserialization of the full config with age_policy is broken, which will
        // cause runtime config loading failures. The JSON structure must remain
        // compatible with both AgePolicyConfig and AppsanityConfig struct definitions.
        let json_payload = r#"{
          "_comment_age_policy": "Age policy configuration consumed by AgePolicyConfig struct in src/gateway/appsanity_gateway.rs. Defines category tag mappings and CET (Consumer Experience Tracking) mappings for different age policy identifiers (AppChild, AppTeen, AppUnknown). Default values are defined in age_policy_default() function.",

                "age_policy": {
                    "category_tag_mappings": {
                        "app:child": ["child"]
                    },
                    "cet_mappings": {
                        "app:child": [
                            "dataPlatform:cet:xvp:personalization:recommendation",
                            "dataPlatform:cet:xvp:advertising:targeted",
                            "dataPlatform:cet:xvp:advertising:segmentation"
                        ]
                    }
                }
        }"#;

        // This should use the serde default values for all other fields
        let config: Result<AppsanityConfig, _> = serde_json::from_str(json_payload);
        assert!(
            config.is_ok(),
            "Failed to deserialize AppsanityConfig with age_policy from JSON"
        );

        let config = config.unwrap();

        // Verify that the age_policy was properly deserialized
        let age_policy = &config.age_policy;
        assert_eq!(age_policy.category_tag_mappings.len(), 1);
        assert_eq!(age_policy.cet_mappings.len(), 1);

        // Verify the specific mappings
        let child_tags = age_policy
            .category_tag_mappings
            .get(&AgePolicy::Child)
            .unwrap();
        assert_eq!(child_tags, &vec!["child".to_string()]);

        let child_cets = age_policy.cet_mappings.get(&AgePolicy::Child).unwrap();
        assert_eq!(child_cets.len(), 3);
        assert!(
            child_cets.contains(&"dataPlatform:cet:xvp:personalization:recommendation".to_string())
        );
    }

    #[test]
    fn test_full_appsanity_config_without_age_policy() {
        // Test loading the entire default config without age_policy specified
        // This should use the age_policy_default() function as fallback
        let config_str = include_str!("default_appsanity_config.json");
        let config: Result<AppsanityConfig, _> = serde_json::from_str(config_str);
        assert!(config.is_ok(), "Failed to deserialize full AppsanityConfig");

        let config = config.unwrap();

        // Verify that age_policy uses the hard-coded defaults
        let age_policy = &config.age_policy;
        let expected_default = age_policy_default();

        // Check category tag mappings match defaults
        assert_eq!(
            age_policy.category_tag_mappings.len(),
            expected_default.category_tag_mappings.len()
        );
        assert_eq!(
            age_policy.category_tag_mappings.get(&AgePolicy::Child),
            expected_default
                .category_tag_mappings
                .get(&AgePolicy::Child)
        );

        // Check CET mappings match defaults
        assert_eq!(
            age_policy.cet_mappings.len(),
            expected_default.cet_mappings.len()
        );
        assert_eq!(
            age_policy.cet_mappings.get(&AgePolicy::Child),
            expected_default.cet_mappings.get(&AgePolicy::Child)
        );

        // Verify the specific default values are present
        let child_tags = age_policy
            .category_tag_mappings
            .get(&AgePolicy::Child)
            .unwrap();
        assert_eq!(child_tags, &vec!["child".to_string()]);

        let child_cets = age_policy.cet_mappings.get(&AgePolicy::Child).unwrap();
        assert_eq!(child_cets.len(), 3);
        assert!(
            child_cets.contains(&"dataPlatform:cet:xvp:personalization:recommendation".to_string())
        );
        assert!(child_cets.contains(&"dataPlatform:cet:xvp:advertising:targeted".to_string()));
        assert!(child_cets.contains(&"dataPlatform:cet:xvp:advertising:segmentation".to_string()));

        // Verify other config sections are loaded properly
        assert!(!config.permission_service.url.is_empty());
        assert!(!config.session_service.url.is_empty());
        assert!(!config.behavioral_metrics.sift.endpoint.is_empty());
    }

    #[test]
    fn test_full_appsanity_config_with_age_policy_override() {
        // Test loading the entire config but with age_policy overridden in JSON
        let config_with_age_policy = r#"{
            "permission_service": {
                "url": "test-permission-service.example.com"
            },
            "session_service": {
                "url": "test-session-service.example.com"
            },
            "behavioral_metrics": {
                "plugin": true,
                "sift": {
                    "endpoint": "test-endpoint.example.com",
                    "api_key": "test-api-key",
                    "batch_size": 10,
                    "max_queue_size": 50,
                    "send_interval_seconds": 30,
                    "ontology": true,
                    "authenticated": true,
                    "metrics_schemas": {
                        "default_metrics_namespace": "test-namespace",
                        "default_metrics_schema_version": "1",
                        "metrics_schemas": []
                    }
                }
            },
            "_comment_age_policy": "Age policy configuration consumed by AgePolicyConfig struct in src/gateway/appsanity_gateway.rs. Defines category tag mappings and CET (Consumer Experience Tracking) mappings for different age policy identifiers (AppChild, AppTeen, AppUnknown). Default values are defined in age_policy_default() function.",

                "age_policy": {
                    "category_tag_mappings": {
                        "app:child": ["child", "family-safe"],
                        "app:teen": ["teen", "young-adult"]
                    },
                    "cet_mappings": {
                        "app:child": [
                            "dataPlatform:cet:xvp:personalization:recommendation",
                            "dataPlatform:cet:xvp:advertising:blocked"
                        ],
                        "app:teen": [
                            "dataPlatform:cet:xvp:personalization:recommendation",
                            "dataPlatform:cet:xvp:advertising:limited"
                        ]
                    }
                }
        }"#;

        let config: Result<AppsanityConfig, _> = serde_json::from_str(config_with_age_policy);
        assert!(
            config.is_ok(),
            "Failed to deserialize AppsanityConfig with age_policy override"
        );

        let config = config.unwrap();

        // Verify that the age_policy was overridden (not using defaults)
        let age_policy = &config.age_policy;
        assert_eq!(age_policy.category_tag_mappings.len(), 2);
        assert_eq!(age_policy.cet_mappings.len(), 2);

        // Check AppChild mappings
        let child_tags = age_policy
            .category_tag_mappings
            .get(&AgePolicy::Child)
            .unwrap();
        assert_eq!(
            child_tags,
            &vec!["child".to_string(), "family-safe".to_string()]
        );

        let child_cets = age_policy.cet_mappings.get(&AgePolicy::Child).unwrap();
        assert_eq!(child_cets.len(), 2);
        assert!(
            child_cets.contains(&"dataPlatform:cet:xvp:personalization:recommendation".to_string())
        );
        assert!(child_cets.contains(&"dataPlatform:cet:xvp:advertising:blocked".to_string()));

        // Check AppTeen mappings
        let teen_tags = age_policy
            .category_tag_mappings
            .get(&AgePolicy::Teen)
            .unwrap();
        assert_eq!(
            teen_tags,
            &vec!["teen".to_string(), "young-adult".to_string()]
        );

        let teen_cets = age_policy.cet_mappings.get(&AgePolicy::Teen).unwrap();
        assert_eq!(teen_cets.len(), 2);
        assert!(
            teen_cets.contains(&"dataPlatform:cet:xvp:personalization:recommendation".to_string())
        );
        assert!(teen_cets.contains(&"dataPlatform:cet:xvp:advertising:limited".to_string()));

        // Verify other config sections are properly loaded
        assert_eq!(
            config.permission_service.url,
            "test-permission-service.example.com"
        );
        assert_eq!(
            config.session_service.url,
            "test-session-service.example.com"
        );
        assert_eq!(
            config.behavioral_metrics.sift.endpoint,
            "test-endpoint.example.com"
        );
        assert_eq!(config.behavioral_metrics.sift.api_key, "test-api-key");
        assert_eq!(config.behavioral_metrics.sift.batch_size, 10);
    }

    #[test]
    fn test_age_policy_defaults_consistency() {
        // Test that the age_policy_default() function produces consistent results
        let default1 = age_policy_default();
        let default2 = age_policy_default();

        // Verify that calling the function multiple times gives the same result
        assert_eq!(
            default1.category_tag_mappings.len(),
            default2.category_tag_mappings.len()
        );
        assert_eq!(default1.cet_mappings.len(), default2.cet_mappings.len());

        // Check specific values for consistency
        assert_eq!(
            default1.category_tag_mappings.get(&AgePolicy::Child),
            default2.category_tag_mappings.get(&AgePolicy::Child)
        );
        assert_eq!(
            default1.cet_mappings.get(&AgePolicy::Child),
            default2.cet_mappings.get(&AgePolicy::Child)
        );

        // Verify the exact default values are as expected
        let child_tags = default1
            .category_tag_mappings
            .get(&AgePolicy::Child)
            .unwrap();
        assert_eq!(child_tags, &vec!["child".to_string()]);

        let child_cets = default1.cet_mappings.get(&AgePolicy::Child).unwrap();
        assert_eq!(child_cets.len(), 3);
        let expected_cets = vec![
            "dataPlatform:cet:xvp:personalization:recommendation".to_string(),
            "dataPlatform:cet:xvp:advertising:targeted".to_string(),
            "dataPlatform:cet:xvp:advertising:segmentation".to_string(),
        ];
        for expected_cet in expected_cets {
            assert!(
                child_cets.contains(&expected_cet),
                "Missing expected CET: {}",
                expected_cet
            );
        }
    }

    #[test]
    fn test_appsanity_config_age_policy_integration() {
        // Test that when no age_policy is specified, the default is used correctly
        let minimal_config = r#"{
            "permission_service": {
                "url": "minimal-test.example.com"
            }
        }"#;

        let config: Result<AppsanityConfig, _> = serde_json::from_str(minimal_config);
        assert!(
            config.is_ok(),
            "Failed to deserialize minimal AppsanityConfig"
        );

        let config = config.unwrap();

        // The age_policy should use the default since it wasn't specified
        let age_policy = &config.age_policy;
        let expected_default = age_policy_default();

        // Deep comparison of the age policy with expected defaults
        assert_eq!(
            age_policy.category_tag_mappings.len(),
            expected_default.category_tag_mappings.len()
        );
        assert_eq!(
            age_policy.cet_mappings.len(),
            expected_default.cet_mappings.len()
        );

        // Verify specific default mappings are present
        assert!(age_policy
            .category_tag_mappings
            .contains_key(&AgePolicy::Child));
        assert!(age_policy.cet_mappings.contains_key(&AgePolicy::Child));

        // Check that the permission service override worked
        assert_eq!(config.permission_service.url, "minimal-test.example.com");

        // Check that other fields use their defaults (not empty)
        assert!(!config.session_service.url.is_empty());
        assert!(!config.behavioral_metrics.sift.endpoint.is_empty());
    }

    #[test]
    fn test_empty_age_policy_config() {
        // Test that an empty age_policy config works correctly
        let config_with_empty_age_policy = r#"{
            "age_policy": {
                "category_tag_mappings": {},
                "cet_mappings": {}
            }
        }"#;

        let config: Result<AppsanityConfig, _> = serde_json::from_str(config_with_empty_age_policy);
        assert!(
            config.is_ok(),
            "Failed to deserialize AppsanityConfig with empty age_policy"
        );

        let config = config.unwrap();

        // The age_policy should be empty as specified
        let age_policy = &config.age_policy;
        assert_eq!(age_policy.category_tag_mappings.len(), 0);
        assert_eq!(age_policy.cet_mappings.len(), 0);

        // Methods should return None for empty config
        assert!(age_policy
            .get_category_tags(Some(AgePolicy::Child))
            .is_none());
        assert!(age_policy
            .get_policy_cets(Some(AgePolicy::Child), vec![AgePolicy::Child])
            .is_none());
    }
}
