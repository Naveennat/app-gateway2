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

pub mod badger_ffi;
pub mod handlers {
    pub mod badger_device_info;
    pub mod badger_info_rpc;
}

pub mod thunder {
    pub mod thunder_session;
}

pub mod authorizer;
pub mod badger_state;
pub mod crypto_util;
pub mod test_utils;

extern crate tokio;

#[tokio::main(worker_threads = 2)]
async fn main() {
    badger_ffi::start_service().await;
}
