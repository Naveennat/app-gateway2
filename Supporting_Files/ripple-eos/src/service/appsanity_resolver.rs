use serde_json::Value;
use std::sync::{Arc, Mutex};
use tokio::sync::{Mutex as TokioMutex, RwLock};

use crate::dpab_core::{
    gateway::DpabDelegate,
    message::{DpabError, DpabRequest, DpabRequestPayload},
};
use queues::CircularBuffer;
use ripple_sdk::log::{debug, info};
use tonic::async_trait;

use crate::{
    gateway::appsanity_gateway::{AppsanityConfig, GrpcClientSession},
    util::{
        cloud_linchpin_monitor::CloudLinchpinMonitor, cloud_periodic_sync::CloudPeriodicSync,
        service_util::create_grpc_client_session,
    },
};

#[async_trait]
pub trait AppsanityDelegate: Send + Sync {
    async fn handle(&mut self, request: DpabRequest);
}

pub struct AppsanityServiceResolver {
    session_service_grpc_client_session: Arc<Mutex<GrpcClientSession>>,
    ad_platform_service_grpc_client_session: Arc<Mutex<GrpcClientSession>>,
    cloud_sync: CloudPeriodicSync,
    cloud_monitor: CloudLinchpinMonitor,
    pub cloud_services: AppsanityConfig,
    eos_rendered: Arc<TokioMutex<CircularBuffer<Value>>>,
    session_token: Arc<RwLock<String>>,
}

impl AppsanityServiceResolver {
    pub fn new(
        cloud_services: AppsanityConfig,
        cloud_sync: CloudPeriodicSync,
        cloud_monitor: CloudLinchpinMonitor,
        eos_rendered: Arc<TokioMutex<CircularBuffer<Value>>>,
        session_token: Arc<RwLock<String>>,
        _firebolt_version: String,
    ) -> AppsanityServiceResolver {
        AppsanityServiceResolver {
            session_service_grpc_client_session: create_grpc_client_session(
                cloud_services.session_service.url.clone(),
            ),
            ad_platform_service_grpc_client_session: create_grpc_client_session(
                cloud_services.ad_platform_service.url.clone(),
            ),
            cloud_sync,
            cloud_monitor,
            cloud_services,
            eos_rendered,
            session_token,
        }
    }

    pub async fn resolve(&mut self, request: DpabRequest) -> Result<(), DpabError> {
        info!("appsanity service resolver {:?}", request);

        let handler: Option<Box<dyn DpabDelegate>> = match request.payload.clone() {
            _ => None,
        };

        match handler {
            Some(mut delegate) => {
                delegate.handle(request).await;
                Ok(())
            }
            None => {
                debug!("No delegate found for default appsanity resolver");
                Err(DpabError::ServiceError)
            }
        }
    }
}
