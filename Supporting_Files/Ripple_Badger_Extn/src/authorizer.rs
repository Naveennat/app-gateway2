use std::collections::HashMap;

use ripple_sdk::api::firebolt::fb_capabilities::{CapabilityRole, FireboltCap, RoleInfo};

pub struct Authorizer;

fn get_role_info(short_cap: &str) -> RoleInfo {
    RoleInfo {
        capability: FireboltCap::short(short_cap),
        role: Some(CapabilityRole::Use),
    }
}

impl Authorizer {
    pub fn get_device_caps() -> Vec<RoleInfo> {
        vec![
            get_role_info("device:info"),
            get_role_info("network:status"),
            get_role_info("device:sku"),
            get_role_info("device:model"),
            get_role_info("device:make"),
        ]
    }

    pub fn get_info_caps() -> Vec<RoleInfo> {
        let mut required_caps = Self::get_device_caps();
        required_caps.push(get_role_info("localization:postal-code"));
        required_caps.push(get_role_info("device:id"));
        required_caps.push(get_role_info("device:uid"));
        required_caps.push(get_role_info("account:id"));
        required_caps.push(get_role_info("account:uid"));
        required_caps.push(get_role_info("device:distributor"));
        required_caps.push(get_role_info("profile:flags"));
        required_caps
    }

    fn check_single_cap(response: &HashMap<String, bool>, cap: &str) -> bool {
        if let Some(v) = response.get(FireboltCap::short(cap).as_str().as_str()) {
            *v
        } else {
            false
        }
    }

    pub fn check_device_info_required(response: &HashMap<String, bool>) -> bool {
        let check_caps: Vec<String> = Self::get_device_caps()
            .into_iter()
            .map(|x| x.capability.as_str())
            .collect();
        for cap in check_caps {
            if let Some(v) = response.get(&cap) {
                if *v {
                    return true;
                }
            }
        }
        false
    }

    pub fn is_device_info_authorized(response: &HashMap<String, bool>) -> bool {
        Self::check_single_cap(response, "device:info")
    }

    pub fn is_network_status_authorized(response: &HashMap<String, bool>) -> bool {
        Self::check_single_cap(response, "network:status")
    }

    pub fn is_device_sku_authorized(response: &HashMap<String, bool>) -> bool {
        Self::check_single_cap(response, "device:sku")
    }

    pub fn is_device_model_authorized(response: &HashMap<String, bool>) -> bool {
        Self::check_single_cap(response, "device:model")
    }

    pub fn is_device_make_authorized(response: &HashMap<String, bool>) -> bool {
        Self::check_single_cap(response, "device:make")
    }

    pub fn is_device_id_authorized(response: &HashMap<String, bool>) -> bool {
        Self::check_single_cap(response, "device:id")
    }

    pub fn is_device_uid_authorized(response: &HashMap<String, bool>) -> bool {
        Self::check_single_cap(response, "device:uid")
    }

    pub fn is_account_id_authorized(response: &HashMap<String, bool>) -> bool {
        Self::check_single_cap(response, "account:id")
    }

    pub fn is_account_uid_authorized(response: &HashMap<String, bool>) -> bool {
        Self::check_single_cap(response, "account:uid")
    }

    pub fn is_device_dist_authorized(response: &HashMap<String, bool>) -> bool {
        Self::check_single_cap(response, "device:distributor")
    }

    pub fn is_postal_authorized(response: &HashMap<String, bool>) -> bool {
        Self::check_single_cap(response, "localization:postal-code")
    }

    pub fn is_profile_flags_authorized(response: &HashMap<String, bool>) -> bool {
        Self::check_single_cap(response, "profile:flags")
    }

    pub fn is_session_required(response: &HashMap<String, bool>) -> bool {
        let caps = vec![
            FireboltCap::short("device:id").as_str(),
            FireboltCap::short("device:uid").as_str(),
            FireboltCap::short("account:id").as_str(),
            FireboltCap::short("account:uid").as_str(),
            FireboltCap::short("device:distributor").as_str(),
        ];

        for cap in caps {
            if let Some(v) = response.get(&cap) {
                if *v {
                    return true;
                }
            }
        }
        false
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_get_device_caps() {
        let caps = Authorizer::get_device_caps();
        assert_eq!(caps.len(), 5);
        assert_eq!(
            caps[0].capability.as_str(),
            "xrn:firebolt:capability:device:info"
        );
        assert_eq!(
            caps[1].capability.as_str(),
            "xrn:firebolt:capability:network:status"
        );
        assert_eq!(
            caps[2].capability.as_str(),
            "xrn:firebolt:capability:device:sku"
        );
        assert_eq!(
            caps[3].capability.as_str(),
            "xrn:firebolt:capability:device:model"
        );
        assert_eq!(
            caps[4].capability.as_str(),
            "xrn:firebolt:capability:device:make"
        );
    }

    #[test]
    fn test_check_single_cap() {
        let mut response = HashMap::new();
        response.insert("xrn:firebolt:capability:device:info".to_string(), true);
        assert!(Authorizer::check_single_cap(&response, "device:info",));
        assert!(!Authorizer::check_single_cap(&response, "network:status"));
    }

    #[test]
    fn test_check_device_info_required() {
        let mut response = HashMap::new();
        response.insert("xrn:firebolt:capability:device:info".to_string(), true);
        assert!(Authorizer::check_device_info_required(&response));
        response.insert("xrn:firebolt:capability:device:info".to_string(), false);
        assert!(!Authorizer::check_device_info_required(&response));
    }

    #[test]
    fn test_is_device_info_authorized() {
        let mut response = HashMap::new();
        response.insert("xrn:firebolt:capability:device:info".to_string(), true);
        assert!(Authorizer::is_device_info_authorized(&response));
        response.insert("xrn:firebolt:capability:device:info".to_string(), false);
        assert!(!Authorizer::is_device_info_authorized(&response));
    }

    #[test]
    fn test_is_network_status_authorized() {
        let mut response = HashMap::new();
        response.insert("xrn:firebolt:capability:network:status".to_string(), true);
        assert!(Authorizer::is_network_status_authorized(&response));
        response.insert("xrn:firebolt:capability:network:status".to_string(), false);
        assert!(!Authorizer::is_network_status_authorized(&response));
    }

    #[test]
    fn test_is_device_sku_authorized() {
        let mut response = HashMap::new();
        response.insert("xrn:firebolt:capability:device:sku".to_string(), true);
        assert!(Authorizer::is_device_sku_authorized(&response));
        response.insert("xrn:firebolt:capability:device:sku".to_string(), false);
        assert!(!Authorizer::is_device_sku_authorized(&response));
    }

    #[test]
    fn test_is_device_model_authorized() {
        let mut response = HashMap::new();
        response.insert("xrn:firebolt:capability:device:model".to_string(), true);
        assert!(Authorizer::is_device_model_authorized(&response));
        response.insert("xrn:firebolt:capability:device:model".to_string(), false);
        assert!(!Authorizer::is_device_model_authorized(&response));
    }

    #[test]
    fn test_is_device_make_authorized() {
        let mut response = HashMap::new();
        response.insert("xrn:firebolt:capability:device:make".to_string(), true);
        assert!(Authorizer::is_device_make_authorized(&response));
        response.insert("xrn:firebolt:capability:device:make".to_string(), false);
        assert!(!Authorizer::is_device_make_authorized(&response));
    }

    #[test]
    fn test_is_device_id_authorized() {
        let mut response = HashMap::new();
        response.insert("xrn:firebolt:capability:device:id".to_string(), true);
        assert!(Authorizer::is_device_id_authorized(&response));
        response.insert("xrn:firebolt:capability:device:id".to_string(), false);
        assert!(!Authorizer::is_device_id_authorized(&response));
    }

    #[test]
    fn test_is_device_uid_authorized() {
        let mut response = HashMap::new();
        response.insert("xrn:firebolt:capability:device:uid".to_string(), true);
        assert!(Authorizer::is_device_uid_authorized(&response));
        response.insert("xrn:firebolt:capability:device:uid".to_string(), false);
        assert!(!Authorizer::is_device_uid_authorized(&response));
    }

    #[test]
    fn test_is_account_id_authorized() {
        let mut response = HashMap::new();
        response.insert("xrn:firebolt:capability:account:id".to_string(), true);
        assert!(Authorizer::is_account_id_authorized(&response));
        response.insert("xrn:firebolt:capability:account:id".to_string(), false);
        assert!(!Authorizer::is_account_id_authorized(&response));
    }

    #[test]
    fn test_is_account_uid_authorized() {
        let mut response = HashMap::new();
        response.insert("xrn:firebolt:capability:account:uid".to_string(), true);
        assert!(Authorizer::is_account_uid_authorized(&response));
        response.insert("xrn:firebolt:capability:account:uid".to_string(), false);
        assert!(!Authorizer::is_account_uid_authorized(&response));
    }

    #[test]
    fn test_is_device_dist_authorized() {
        let mut response = HashMap::new();
        response.insert(
            "xrn:firebolt:capability:device:distributor".to_string(),
            true,
        );
        assert!(Authorizer::is_device_dist_authorized(&response));
        response.insert(
            "xrn:firebolt:capability:device:distributor".to_string(),
            false,
        );
        assert!(!Authorizer::is_device_dist_authorized(&response));
    }

    #[test]
    fn test_is_postal_authorized() {
        let mut response = HashMap::new();
        response.insert(
            "xrn:firebolt:capability:localization:postal-code".to_string(),
            true,
        );
        assert!(Authorizer::is_postal_authorized(&response));
        response.insert(
            "xrn:firebolt:capability:localization:postal-code".to_string(),
            false,
        );
        assert!(!Authorizer::is_postal_authorized(&response));
    }

    #[test]
    fn test_is_profile_flags_authorized() {
        let mut response = HashMap::new();
        response.insert("xrn:firebolt:capability:profile:flags".to_string(), true);
        assert!(Authorizer::is_profile_flags_authorized(&response));
        response.insert("xrn:firebolt:capability:profile:flags".to_string(), false);
        assert!(!Authorizer::is_profile_flags_authorized(&response));
    }

    #[test]
    fn test_is_session_required() {
        let mut response = HashMap::new();
        response.insert("xrn:firebolt:capability:device:id".to_string(), true);
        assert!(Authorizer::is_session_required(&response));
        response.insert("xrn:firebolt:capability:device:id".to_string(), false);
        assert!(!Authorizer::is_session_required(&response));
    }

    #[test]
    fn test_get_info_caps() {
        let caps = Authorizer::get_info_caps();
        assert_eq!(caps.len(), 12);
        assert_eq!(
            caps[0].capability.as_str(),
            "xrn:firebolt:capability:device:info"
        );
        assert_eq!(
            caps[1].capability.as_str(),
            "xrn:firebolt:capability:network:status"
        );
        assert_eq!(
            caps[2].capability.as_str(),
            "xrn:firebolt:capability:device:sku"
        );
        assert_eq!(
            caps[3].capability.as_str(),
            "xrn:firebolt:capability:device:model"
        );
        assert_eq!(
            caps[4].capability.as_str(),
            "xrn:firebolt:capability:device:make"
        );
        assert_eq!(
            caps[5].capability.as_str(),
            "xrn:firebolt:capability:localization:postal-code"
        );
        assert_eq!(
            caps[6].capability.as_str(),
            "xrn:firebolt:capability:device:id"
        );
        assert_eq!(
            caps[7].capability.as_str(),
            "xrn:firebolt:capability:device:uid"
        );
        assert_eq!(
            caps[8].capability.as_str(),
            "xrn:firebolt:capability:account:id"
        );
        assert_eq!(
            caps[9].capability.as_str(),
            "xrn:firebolt:capability:account:uid"
        );
        assert_eq!(
            caps[10].capability.as_str(),
            "xrn:firebolt:capability:device:distributor"
        );
        assert_eq!(
            caps[11].capability.as_str(),
            "xrn:firebolt:capability:profile:flags"
        );
    }

    #[test]
    fn test_check_single_cap_not_found() {
        let response = HashMap::new();
        assert!(!Authorizer::check_single_cap(&response, "nonexistent:cap"));
    }

    #[test]
    fn test_check_single_cap_false() {
        let mut response = HashMap::new();
        response.insert("xrn:firebolt:capability:device:info".to_string(), false);
        assert!(!Authorizer::check_single_cap(&response, "device:info"));
    }

    #[test]
    fn test_check_device_info_required_false() {
        let response = HashMap::new();
        assert!(!Authorizer::check_device_info_required(&response));
    }

    #[test]
    fn test_is_session_required_false() {
        let response = HashMap::new();
        assert!(!Authorizer::is_session_required(&response));
    }
}
