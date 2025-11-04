use crate::state::distributor_state::DistributorState;
use thunder_ripple_sdk::ripple_sdk::{
    api::distributor::distributor_usergrants::*,
    async_trait::async_trait,
    extn::{
        client::{
            extn_client::ExtnClient,
            extn_processor::{
                DefaultExtnStreamer, ExtnRequestProcessor, ExtnStreamProcessor, ExtnStreamer,
            },
        },
        extn_client_message::{ExtnMessage, ExtnResponse},
    },
    tokio::sync::mpsc,
    utils::error::RippleError,
};

pub struct DistributorStoreUserGrantsProcessor {
    state: DistributorState,
    streamer: DefaultExtnStreamer,
}

impl DistributorStoreUserGrantsProcessor {
    pub fn new(state: DistributorState) -> DistributorStoreUserGrantsProcessor {
        DistributorStoreUserGrantsProcessor {
            state,
            streamer: DefaultExtnStreamer::new(),
        }
    }
}

impl ExtnStreamProcessor for DistributorStoreUserGrantsProcessor {
    type STATE = DistributorState;
    type VALUE = UserGrantsCloudStoreRequest;

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
impl ExtnRequestProcessor for DistributorStoreUserGrantsProcessor {
    fn get_client(&self) -> ExtnClient {
        self.state.get_client()
    }

    async fn process_request(state: Self::STATE, msg: ExtnMessage, val: Self::VALUE) -> bool {
        match val {
            UserGrantsCloudStoreRequest::GetCloudUserGrants(_get_cloud_usergrants_params) => {
                todo!()
            }
            UserGrantsCloudStoreRequest::SetCloudUserGrants(set_cloud_usergrants_params) => {
                let response = state
                    .privacy_service
                    .set_user_grant(&set_cloud_usergrants_params)
                    .await;
                if response.is_ok() {
                    return Self::respond(state.get_client(), msg, ExtnResponse::None(()))
                        .await
                        .is_ok();
                }
            }
        }
        Self::handle_error(state.get_client(), msg, RippleError::ExtnError).await
    }
}
