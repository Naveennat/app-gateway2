// Copyright 2023 Comcast Cable Communications Management, LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
//

use jsonrpsee::{
    core::{async_trait, RpcResult},
    proc_macros::rpc,
};
use thunder_ripple_sdk::{
    client::{
        device_operator::{DeviceCallRequest, DeviceChannelParams, DeviceOperator},
        thunder_plugin::ThunderPlugin,
    },
    ripple_sdk::{
        api::{gateway::rpc_gateway_api::CallContext, session::ProvisionRequest},
        log::{debug, error, info},
    },
};

use crate::{
    extn::thunder::thunder_session::ThunderSessionService, rpc::rpc_utils::rpc_err,
    state::distributor_state::DistributorState,
};

#[rpc(server)]
pub trait Device {
    #[method(name = "device.provision")]
    async fn provision(
        &self,
        ctx: CallContext,
        provision_request: ProvisionRequest,
    ) -> RpcResult<()>;
    #[method(name = "device.id")]
    async fn id(&self, ctx: CallContext) -> RpcResult<String>;
    #[method(name = "device.distributor")]
    async fn distributor(&self, ctx: CallContext) -> RpcResult<String>;
}

pub struct DeviceImpl {
    pub state: DistributorState,
}

impl DeviceImpl {
    pub fn new(state: &DistributorState) -> Self {
        Self {
            state: state.clone(),
        }
    }

    async fn provision(
        &self,
        mut _ctx: CallContext,
        provision_request: ProvisionRequest,
    ) -> RpcResult<()> {
        if provision_request.distributor_id.is_none() {
            error!("Provision request missing distributor_id.");
            return Err(rpc_err("Provision request must include a distributor_id."));
        }

        info!("Received provision device request: {:?}", provision_request);
        let ref_session = self.state.get_account_session();
        if let Some(mut session) = ref_session {
            session.device_id = provision_request.device_id.clone();
            session.account_id = provision_request.account_id.clone();
            if let Some(id) = provision_request.distributor_id.clone() {
                session.id = id;
            }
            // update the cached distributor session
            self.state.update_account_session(&session);
        } else {
            error!("Account session not found in cache when trying to provision device.");
            match ThunderSessionService::get_account_session(&self.state.get_thunder()).await {
                Ok(mut session) => {
                    session.device_id = provision_request.device_id.clone();
                    session.account_id = provision_request.account_id.clone();
                    if let Some(id) = provision_request.distributor_id.clone() {
                        session.id = id;
                    }

                    // update the cached session if we can retrieve it from Thunder
                    self.state.update_account_session(&session);
                }
                Err(e) => {
                    error!(
                        "Failed to retrieve account session from Thunder to provision device: {}",
                        e
                    );
                }
            }
        }

        if ThunderSessionService::set_provision(self.state.get_thunder(), provision_request).await {
            Ok(())
        } else {
            Err(rpc_err("Provision Status error response TBD"))
        }
    }
}

#[async_trait]
impl DeviceServer for DeviceImpl {
    async fn provision(
        &self,
        ctx: CallContext,
        provision_request: ProvisionRequest,
    ) -> RpcResult<()> {
        info!("Device.Provision");
        self.provision(ctx, provision_request).await
    }

    async fn id(&self, _ctx: CallContext) -> RpcResult<String> {
        info!("Device.Id");
        get_device_id(&self.state).await
    }

    async fn distributor(&self, _ctx: CallContext) -> RpcResult<String> {
        info!("Device.Distributor");
        if let Some(session) = self.state.get_account_session() {
            Ok(session.id)
        } else {
            Err(rpc_err("Distributor ID not found"))
        }
    }
}

pub(crate) async fn get_device_id(state: &DistributorState) -> RpcResult<String> {
    if let Some(session) = state.get_account_session() {
        return Ok(session.device_id.clone());
    }

    match get_mac(state).await.as_str() {
        "" => Err(rpc_err("device.info.mac_address error")),
        mac => Ok(filter_mac(mac.to_string())),
    }
}

fn filter_mac(mac_address: String) -> String {
    let filtered = mac_address.replace(':', "");
    if filtered.len() != 12 || !filtered.chars().all(|c| c.is_ascii_hexdigit()) {
        error!("Invalid MAC address format for mac{}", mac_address);
    }
    filtered
}

pub(crate) async fn get_mac(state: &DistributorState) -> String {
    let client = state.get_thunder_client();
    let request = DeviceCallRequest {
        method: ThunderPlugin::System.method("getDeviceInfo"),
        params: Some(DeviceChannelParams::Json(String::from(
            "{\"params\": [\"estb_mac\"]}",
        ))),
    };
    let resp = client.call(request).await;
    debug!("{}", resp.message);
    let resp = resp.message.get("estb_mac");
    if resp.is_none() {
        return "".to_string();
    }
    resp.unwrap()
        .as_str()
        .unwrap()
        .trim_matches('"')
        .to_string()
}
