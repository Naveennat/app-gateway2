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

use crate::state::distributor_state::DistributorState;
use jsonrpsee::{
    core::{async_trait, RpcResult},
    proc_macros::rpc,
};
use thunder_ripple_sdk::ripple_sdk::{
    api::{
        gateway::rpc_gateway_api::CallContext, storage_manager::StorageManager,
        storage_property::StorageProperty,
    },
    log::info,
};

#[rpc(server)]
pub trait Privacy {
    #[method(name = "badger.limitAdTracking")]
    async fn badger_limit_ad_tracking(&self, ctx: CallContext) -> RpcResult<bool>;
}

#[derive(Clone)]
pub struct PrivacyImpl {
    pub state: DistributorState,
}

impl PrivacyImpl {
    pub fn new(state: &DistributorState) -> Self {
        Self {
            state: state.clone(),
        }
    }

    async fn badger_limit_ad_tracking(&self, _ctx: CallContext) -> RpcResult<bool> {
        let allow_app_content_ad_targeting =
            StorageManager::get_bool(&self.state, StorageProperty::AllowAppContentAdTargeting)
                .await
                .unwrap_or(true);
        // Storage property is allow AdTargeting, but the query is for limit(deny) AdTargeting
        return Ok(!allow_app_content_ad_targeting);
    }
}

#[async_trait]
impl PrivacyServer for PrivacyImpl {
    async fn badger_limit_ad_tracking(&self, ctx: CallContext) -> RpcResult<bool> {
        info!("badger.limitAdTracking");
        self.badger_limit_ad_tracking(ctx).await
    }
}
