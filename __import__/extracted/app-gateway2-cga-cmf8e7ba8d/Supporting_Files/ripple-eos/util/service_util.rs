use std::sync::{Arc, Mutex};

use ripple_sdk::{
    api::{firebolt::fb_discovery::AgePolicy, gateway::rpc_gateway_api::CallContext},
    service::{service_client::ServiceClient, service_message::JsonRpcMessage},
};
use thunder_ripple_sdk::ripple_sdk::api::session::AccountSession;
use tonic::{
    transport::{Channel, ClientTlsConfig},
    Request,
};

use crate::{
    extn::eos_ffi::EOS_DISTRIBUTOR_SERVICE_ID, gateway::appsanity_gateway::GrpcClientSession,
};

pub fn decorate_request_with_account_session(req: &mut Request<()>, session: &AccountSession) {
    let bearer = format!("Bearer {}", session.token);
    req.metadata_mut()
        .insert("authorization", (bearer.as_str().parse()).unwrap());
    req.metadata_mut()
        .insert("deviceid", (session.device_id.as_str().parse()).unwrap());
    req.metadata_mut()
        .insert("accountid", (session.account_id.as_str().parse()).unwrap());
    req.metadata_mut()
        .insert("partnerid", (session.id.as_str().parse()).unwrap());
}

pub fn create_lazy_tls_channel(service_url: String) -> Channel {
    tonic::transport::Channel::from_shared(format!("https://{}", service_url.clone()))
        .unwrap()
        .tls_config(ClientTlsConfig::new().domain_name(service_url.clone()))
        .unwrap()
        .connect_lazy()
}

pub fn create_grpc_client_session(service_url: String) -> Arc<Mutex<GrpcClientSession>> {
    Arc::new(Mutex::new(GrpcClientSession::new(service_url.clone())))
}

/*
non tls version for testing with locally running microservices */
pub fn create_lazy_insecure_channel(service_url: String) -> Channel {
    tonic::transport::Channel::from_shared(format!("http://{}", service_url.clone()))
        .unwrap()
        .connect_lazy()
}

pub async fn get_age_policy_identifiers(
    service_client: Option<ServiceClient>,
    call_context: CallContext,
) -> Vec<AgePolicy> {
    let aliases = match service_client.clone() {
        Some(client) => {
            let mut mutant = client.clone();
            mutant
                .request_with_timeout_main(
                    "account.policyIdentifierAlias".to_string(),
                    None,
                    Some(&call_context.clone()),
                    5000,
                    EOS_DISTRIBUTOR_SERVICE_ID.to_string(),
                )
                .await
        }
        None => return Vec::new(),
    };
    match aliases {
        Ok(service_message) => match service_message.message {
            JsonRpcMessage::Success(response) => {
                log::debug!(
                    "got age_policy_identifier_aliases from platform_state: {}",
                    &response.result
                );
                let result: Vec<AgePolicy> =
                    serde_json::from_value(response.result).unwrap_or_default();

                return result;
            }
            _ => return Vec::new(),
        },
        Err(_) => return Vec::new(),
    }
}
