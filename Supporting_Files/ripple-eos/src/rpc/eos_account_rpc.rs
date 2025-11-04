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

use crate::{
    extn::thunder::thunder_session::ThunderSessionService, rpc::rpc_utils::rpc_err,
    state::distributor_state::DistributorState,
};
use jsonrpsee::{
    core::{async_trait, RpcResult},
    proc_macros::rpc,
};
use thunder_ripple_sdk::ripple_sdk::{
    api::{
        gateway::rpc_gateway_api::CallContext, session::AccountSessionTokenRequest,
        storage_manager::StorageManager, storage_property::StoragePropertyData,
    },
    log::info,
    uuid::Uuid,
};

const KEY_FIREBOLT_ACCOUNT_UID: &str = "fireboltAccountUid";
const UID_SCOPE: &str = "device";

#[rpc(server)]
pub trait Account {
    #[method(name = "account.session")]
    async fn session(&self, ctx: CallContext, a_t_r: AccountSessionTokenRequest) -> RpcResult<()>;
    #[method(name = "account.id")]
    async fn id(&self, ctx: CallContext) -> RpcResult<String>;
    #[method(name = "account.uid")]
    async fn uid(&self, ctx: CallContext) -> RpcResult<String>;
}

#[derive(Clone)]
pub struct AccountImpl {
    pub state: DistributorState,
}

impl AccountImpl {
    pub fn new(state: &DistributorState) -> Self {
        Self {
            state: state.clone(),
        }
    }

    async fn session(
        &self,
        _ctx: &CallContext,
        a_t_r: AccountSessionTokenRequest,
    ) -> RpcResult<()> {
        self.state.update_token(a_t_r.token.clone());

        // thunder call to set the session token in thunder - account.setServiceAccessToken
        if ThunderSessionService::set_session(self.state.get_thunder(), a_t_r).await {
            Ok(())
        } else {
            Err(rpc_err("failed to set session"))
        }
    }

    async fn uid(&self, ctx: CallContext) -> RpcResult<String> {
        let uid: String;
        let mut data = StoragePropertyData {
            scope: Some(UID_SCOPE.to_string()),
            key: KEY_FIREBOLT_ACCOUNT_UID,
            namespace: ctx.app_id.clone(),
            value: String::new(),
        };

        if let Ok(id) = StorageManager::get_string_for_scope(&self.state, &data).await {
            uid = id;
        } else {
            // Using app_id as namespace will result in different uid for each app
            data.value = Uuid::new_v4().to_string();
            StorageManager::set_string_for_scope(&self.state, &data, None).await?;
            uid = data.value;
        }
        Ok(uid)
    }
}

#[async_trait]
impl AccountServer for AccountImpl {
    async fn session(&self, ctx: CallContext, a_t_r: AccountSessionTokenRequest) -> RpcResult<()> {
        info!("Account.Session");
        self.session(&ctx, a_t_r).await
    }

    async fn id(&self, _ctx: CallContext) -> RpcResult<String> {
        info!("Account.id");
        match &self.state.get_account_session() {
            Some(session) => Ok(session.account_id.clone()),
            None => Err(rpc_err("Account.id: failed to get account session")),
        }
    }

    async fn uid(&self, ctx: CallContext) -> RpcResult<String> {
        self.uid(ctx).await
    }
}
