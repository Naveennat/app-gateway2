use std::sync::{Arc, Mutex};

use thunder_ripple_sdk::ripple_sdk::log::error;
use tonic::transport::Channel;

use crate::gateway::appsanity_gateway::GrpcClientSession;
use crate::message::DpabError;
use crate::model::auth::GetOttTokenResponse;
use crate::util::service_util::decorate_request_with_account_session;
use ottx_protos::ott_token::ott_token_service_client::OttTokenServiceClient;
use ottx_protos::ott_token::PlatformTokenRequest;
use thunder_ripple_sdk::ripple_sdk::api::session::AccountSession;
use tonic::Request;
use uuid::Uuid;

#[derive(Clone)]
pub struct OttTokenService {
    grpc_client_session: Arc<Mutex<GrpcClientSession>>,
}

impl OttTokenService {
    fn get_channel(&self) -> Result<Channel, DpabError> {
        self.grpc_client_session.lock().unwrap().get_grpc_channel()
    }

    pub async fn get_ott_token(
        &self,
        account_session: &AccountSession,
        app_id: String,
        xact: String,
    ) -> Result<GetOttTokenResponse, DpabError> {
        let channel = match self.get_channel() {
            Ok(c) => c,
            Err(e) => return Err(e),
        };
        let mut client: OttTokenServiceClient<_> =
            OttTokenServiceClient::with_interceptor(channel, move |mut req: Request<()>| {
                decorate_request_with_account_session(&mut req, account_session);
                Ok(req)
            });

        let request = tonic::Request::new(PlatformTokenRequest {
            xact,
            sat: account_session.token.clone(),
            app_id,
        });

        let response = client.platform_token(request).await;
        match response {
            Ok(payload) => {
                let token_response = payload.into_inner();
                let ott_token = GetOttTokenResponse {
                    access_token: token_response.platform_token,
                    token_type: token_response.token_type,
                    scope: token_response.scope,
                    expires_in: token_response.expires_in,
                    tid: Uuid::new_v4().to_string(),
                };
                Ok(ott_token)
            }
            Err(err) => {
                error!("Server returned {:?}", err);
                Err(DpabError::ServiceError)
            }
        }
    }

    pub fn new_from(grpc_client_session: Arc<Mutex<GrpcClientSession>>) -> OttTokenService {
        OttTokenService {
            grpc_client_session,
        }
    }
}

#[cfg(test)]
pub mod tests {
    use super::*;
    use thunder_ripple_sdk::ripple_sdk::Mockable;
    impl Mockable for OttTokenService {
        fn mock() -> Self {
            OttTokenService {
                grpc_client_session: Arc::new(Mutex::new(GrpcClientSession::mock("some_url"))),
            }
        }
    }
}
