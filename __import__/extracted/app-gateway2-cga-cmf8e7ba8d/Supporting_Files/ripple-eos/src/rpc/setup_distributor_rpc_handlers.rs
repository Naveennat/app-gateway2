use crate::rpc::eos_authentication_rpc::{AuthenticationImpl, AuthenticationServer};
use jsonrpsee::core::server::rpc_module::Methods;
use thunder_ripple_sdk::ripple_sdk::extn::extn_id::{ExtnClassId, ExtnId};
use thunder_ripple_sdk::ripple_sdk::processor::rpc_request_processor::RPCRequestProcessor;

use crate::rpc::eos_account_rpc::{AccountImpl, AccountServer};
use crate::rpc::eos_device_rpc::{DeviceImpl, DeviceServer};
use crate::state::distributor_state::DistributorState;

use super::eos_advertising_rpc::{AdvertisingImpl, AdvertisingServer};
use super::eos_discovery_rpc::{DiscoveryImpl, DiscoveryServer};
use super::eos_metrics_management_rpc::{MetricsManagementImpl, MetricsManagementServer};
use super::eos_metrics_rpc::{MetricsImpl, MetricsServer};
use super::eos_privacy_rpc::{PrivacyImpl, PrivacyServer};

fn get_rpc_extns(state: DistributorState) -> Methods {
    let mut methods = Methods::new();
    let _ = methods.merge(AdvertisingImpl::new(&state).into_rpc());
    let _ = methods.merge(DiscoveryImpl::new(&state).into_rpc());
    let _ = methods.merge(DeviceImpl::new(&state).into_rpc());
    let _ = methods.merge(AccountImpl::new(&state).into_rpc());
    let _ = methods.merge(AuthenticationImpl::new(&state).into_rpc());
    methods
}

pub fn get_rpc_methods_for_service(state: &DistributorState) -> Methods {
    let mut methods = Methods::new();
    let _ = methods.merge(MetricsManagementImpl::new(&state).into_rpc());
    let _ = methods.merge(MetricsImpl::new(&state).into_rpc());
    let _ = methods.merge(PrivacyImpl::new(state).into_rpc());
    methods
}

pub fn setup_distributor_rpc_handlers(state: DistributorState) {
    let methods = get_rpc_extns(state.clone());
    let processor = RPCRequestProcessor::new(
        state.get_client(),
        methods,
        ExtnId::new_channel(ExtnClassId::Distributor, "eos".into()),
    );
    state.get_client().add_request_processor(processor);
}
