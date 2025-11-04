use crate::state::distributor_state::DistributorState;
use thunder_ripple_sdk::ripple_sdk::{
    api::distributor::distributor_privacy::{PrivacyCloudRequest, PrivacyResponse},
    async_trait::async_trait,
    extn::{
        client::{
            extn_client::ExtnClient,
            extn_processor::{
                DefaultExtnStreamer, ExtnRequestProcessor, ExtnStreamProcessor, ExtnStreamer,
            },
        },
        extn_client_message::{ExtnMessage, ExtnPayloadProvider, ExtnResponse},
    },
    tokio::sync::mpsc,
    utils::error::RippleError,
};

pub struct DistributorPrivacyProcessor {
    state: DistributorState,
    streamer: DefaultExtnStreamer,
}

impl DistributorPrivacyProcessor {
    pub fn new(state: DistributorState) -> DistributorPrivacyProcessor {
        DistributorPrivacyProcessor {
            state,
            streamer: DefaultExtnStreamer::new(),
        }
    }
}

impl ExtnStreamProcessor for DistributorPrivacyProcessor {
    type STATE = DistributorState;
    type VALUE = PrivacyCloudRequest;

    fn get_state(&self) -> Self::STATE {
        self.state.clone()
    }

    fn receiver(&mut self) -> mpsc::Receiver<ExtnMessage> {
        self.streamer.receiver()
    }

    fn sender(&self) -> mpsc::Sender<ExtnMessage> {
        self.streamer.sender()
    }
}

#[async_trait]
impl ExtnRequestProcessor for DistributorPrivacyProcessor {
    fn get_client(&self) -> ExtnClient {
        self.state.get_client().clone()
    }

    async fn process_request(state: Self::STATE, msg: ExtnMessage, val: Self::VALUE) -> bool {
        match val {
            PrivacyCloudRequest::GetProperty(get_property_params) => {
                let service = state.privacy_service.clone();
                let result = service.get_property(get_property_params).await;
                if let Ok(privacy_response) = result {
                    if let PrivacyResponse::Bool(val) = privacy_response {
                        return Self::respond(
                            state.get_client().clone(),
                            msg,
                            ExtnResponse::Boolean(val),
                        )
                        .await
                        .is_ok();
                    }
                }
            }

            PrivacyCloudRequest::GetProperties(account_session) => {
                let service = state.privacy_service.clone();
                let result = service.get_properties(account_session).await;
                if let Ok(privacy_response) = result {
                    if let PrivacyResponse::Settings(privacy_settings) = privacy_response {
                        if let Some(response) = privacy_settings.get_extn_payload().as_response() {
                            return Self::respond(state.get_client().clone(), msg, response)
                                .await
                                .is_ok();
                        }
                    }
                }
            }
            PrivacyCloudRequest::SetProperty(set_property_params) => {
                let service = state.privacy_service.clone();
                let result = service.set_property(set_property_params.clone()).await;
                if let Ok(_) = result {
                    // update local state info for privacy
                    state.update_privacy_setting(set_property_params);
                    return Self::ack(state.get_client().clone(), msg).await.is_ok();
                }
            }
            PrivacyCloudRequest::GetPartnerExclusions(account_session) => {
                let service = state.privacy_service.clone();
                let result = service.get_partner_exclusions(account_session).await;
                if let Ok(exclusion_policy) = result {
                    if let Some(response) = exclusion_policy.get_extn_payload().as_response() {
                        return Self::respond(state.get_client().clone(), msg, response)
                            .await
                            .is_ok();
                    }
                }
            }
        }
        Self::handle_error(state.get_client().clone(), msg, RippleError::ExtnError).await
    }
}
