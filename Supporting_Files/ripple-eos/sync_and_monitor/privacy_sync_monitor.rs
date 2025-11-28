// This is a sample implementation. This should be replaced with the actual implementation

use crate::util::cloud_sync_monitor_utils::SyncAndMonitorProcessor;
use async_trait::async_trait;
pub struct PrivacySyncMonitorService;
impl PrivacySyncMonitorService {
    pub fn new() -> Self {
        PrivacySyncMonitorService {}
    }
}
#[async_trait]
impl SyncAndMonitorProcessor for PrivacySyncMonitorService {
    fn get_properties(&self) -> Vec<String> {
        let supported_properties = vec![
            "xcal:continueWatching".to_string(),
            "xcal:unentitledContinueWatching".to_string(),
            "xcal:watchHistory".to_string(),
            "xcal:productAnalytics".to_string(),
            "xcal:personalization".to_string(),
            "xcal:unentitledPersonalization".to_string(),
            "xcal:remoteDiagnostics".to_string(),
            "xcal:primaryContentAdTargeting".to_string(),
            "xcal:primaryBrowseAdTargeting".to_string(),
            "xcal:appContentAdTargeting".to_string(),
            "xcal:acr".to_string(),
            "xcal:cameraAnalytics".to_string(),
        ];
        supported_properties
    }
}
