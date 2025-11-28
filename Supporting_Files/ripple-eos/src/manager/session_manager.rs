use std::time::Duration;

use crate::{
    extn::thunder::{
        events::comcast_thunder_event_handlers::SessionTokenChangeHandler,
        thunder_session::ThunderSessionService,
    },
    state::distributor_state::DistributorState,
};
use log::{debug, error, info};
use thunder_ripple_sdk::client::device_operator::{
    DeviceOperator, DeviceResponseMessage, DeviceSubscribeRequest,
};
use thunder_ripple_sdk::{
    events::thunder_event_processor::ThunderEventHandlerProvider,
    ripple_sdk::api::session::AccountSession,
};
use tokio::sync::mpsc;

async fn setup_session_listener(distributor_state: &DistributorState) {
    debug!("Setting up session listener");
    let (sub_tx, mut sub_rx) = mpsc::channel::<DeviceResponseMessage>(5);
    let client = distributor_state.get_thunder().get_thunder_client();
    let event_name = SessionTokenChangeHandler::event_name();
    let request = DeviceSubscribeRequest {
        module: SessionTokenChangeHandler::module(),
        event_name: event_name.clone(),
        params: None,
        sub_id: None,
    };

    if let Err(e) = client.subscribe(request, sub_tx).await {
        error!("Failed to subscribe to {}: {:?}", event_name, e);
    } else {
        info!("Successfully subscribed to {}", event_name);
    }

    let dist_c = distributor_state.clone();
    let mut first_event = true;
    tokio::spawn(async move {
        while let Some(_) = sub_rx.recv().await {
            info!("Got Service Access Token Changed event");
            // No session yet
            if dist_c.get_account_session().is_none() {
                // Could be the first event
                if first_event {
                    // set the first event to false until the backoff expires
                    first_event = false;
                    // need to get the entire account session
                    if token_available_check_session(dist_c.clone()).await.is_err() {
                        first_event = true;
                    }
                }
            } else {
                handle_token_update(dist_c.clone()).await
            }
        }
    });
}

// When SAT Token is set the other fields like device id, account Id and partner Id might be delayed
// This presents a problem to Ripple where we cannot add these entries into metrics context
// Preference here is to have all the data ready and have a RDK based back off policy to
// exponentially back off when data is not available during retry
// Retries are exponential in terms of back off 11 retries will happen over a period of 1.4 secs
async fn token_available_check_session(distributor_state: DistributorState) -> Result<(), u64> {
    let mut previous_back_off = 0;
    let mut current_back_off = 10;
    const TOTAL_BACKOFF_ALLOWED: i32 = 11;
    let mut total_times_backed_off = 0;
    loop {
        if let Ok(v) = ThunderSessionService::get_account_session(&distributor_state.thunder).await
        {
            handle_first_session_load(distributor_state, v);
            return Ok(());
        } else {
            let temp = current_back_off;
            current_back_off = current_back_off + previous_back_off;
            previous_back_off = temp;
            debug!("backing_off {}", current_back_off);
            total_times_backed_off += 1;
            if total_times_backed_off <= TOTAL_BACKOFF_ALLOWED {
                tokio::time::sleep(Duration::from_millis(current_back_off)).await;
            } else {
                break;
            }
        }
    }
    Err(previous_back_off)
}

pub async fn handle_session(distributor_state: DistributorState) {
    // subscribe for session
    // Setup the Session listener is session is enabled on the manifest
    setup_session_listener(&distributor_state).await;

    if let Ok(session) =
        ThunderSessionService::get_account_session(&distributor_state.thunder).await
    {
        handle_first_session_load(distributor_state, session);
    }
}

fn handle_first_session_load(distributor_state: DistributorState, session: AccountSession) {
    distributor_state.update_account_session(&session);
    tokio::spawn(async move {
        distributor_state.handle_first_account_session_update(&session);
    });
}

async fn handle_token_update(distributor_state: DistributorState) {
    let token = ThunderSessionService::get_sat_token(&distributor_state.get_thunder()).await;
    if !token.is_empty() {
        distributor_state.update_token(token);
    }
}

#[cfg(test)]
pub mod tests {
    use std::{
        sync::{
            atomic::{AtomicU64, Ordering},
            Arc,
        },
        time::Duration,
    };

    use crate::{
        gateway::appsanity_gateway::AppsanityConfig,
        manager::session_manager::{handle_session, token_available_check_session},
        service::{ott_token::OttTokenService, thor_permission::ThorPermissionService},
        state::distributor_state::{AuthService, DistributorState},
        util::channel_util::oneshot_send_and_log,
    };
    use log::debug;
    use serde_json::json;
    use thunder_ripple_sdk::{
        client::{device_operator::DeviceResponseMessage, thunder_client::ThunderCallMessage},
        tests::mock_thunder_controller::{CustomHandler, MockThunderController},
    };
    use thunder_ripple_sdk::{
        ripple_sdk::{api::manifest::device_manifest::DeviceManifest, Mockable},
        tests::mock_thunder_controller::MockThunderSubscriberfn,
    };

    static LOCAL_TEST_ID: AtomicU64 = AtomicU64::new(0);

    fn increment() {
        LOCAL_TEST_ID.fetch_add(1, Ordering::Relaxed);
    }

    fn reset() {
        LOCAL_TEST_ID.store(0, Ordering::Relaxed);
    }
    // Fully activated and session available usecase
    #[tokio::test]
    async fn test_already_activated_session() {
        let mut ch = CustomHandler::default();
        ch.custom_request_handler.insert(
            "org.rdk.AuthService.getXDeviceId".to_owned(),
            Arc::new(|msg: ThunderCallMessage| {
                oneshot_send_and_log(
                    msg.callback,
                    DeviceResponseMessage::call(
                        json!({"success" : true, "xDeviceId": "someDeviceId" }),
                    ),
                    "",
                );
            }),
        );

        ch.custom_request_handler.insert(
            "org.rdk.AuthService.getServiceAccountId".to_owned(),
            Arc::new(|msg: ThunderCallMessage| {
                oneshot_send_and_log(
                    msg.callback,
                    DeviceResponseMessage::call(
                        json!({"success" : true, "serviceAccountId": "someAccountId" }),
                    ),
                    "",
                );
            }),
        );

        ch.custom_request_handler.insert(
            "org.rdk.AuthService.getDeviceId".to_owned(),
            Arc::new(|msg: ThunderCallMessage| {
                oneshot_send_and_log(
                    msg.callback,
                    DeviceResponseMessage::call(
                        json!({"success" : true, "partnerId": "somePartnerId" }),
                    ),
                    "",
                );
            }),
        );

        ch.custom_request_handler.insert(
            "org.rdk.AuthService.getServiceAccessToken".to_owned(),
            Arc::new(|msg: ThunderCallMessage| {
                oneshot_send_and_log(
                    msg.callback,
                    DeviceResponseMessage::call(json!({"success" : true, "token": "someToken" })),
                    "",
                );
            }),
        );
        let thunder = MockThunderController::get_thunder_state_mock_with_handler(Some(ch));
        let distributor_state = DistributorState::new(
            &DeviceManifest::default(),
            thunder,
            AppsanityConfig::mock(),
            AuthService::new(ThorPermissionService::mock(), OttTokenService::mock()),
            None,
        );
        handle_session(distributor_state.clone()).await;
        let session = distributor_state.get_account_session().unwrap();
        assert!(session.token.eq("someToken"));
        assert!(session.id.eq("somePartnerId"));
        assert!(session.account_id.eq("someAccountId"));
        assert!(session.device_id.eq("someDeviceId"));
    }

    // Session is not available on startup and gets populated over time

    #[tokio::test]
    async fn no_token_on_startup() {
        reset();
        let mut ch = CustomHandler::default();
        ch.custom_request_handler.insert(
            "org.rdk.AuthService.getXDeviceId".to_owned(),
            Arc::new(|msg: ThunderCallMessage| {
                let result = { LOCAL_TEST_ID.load(Ordering::Relaxed) > 0 };
                let r = if result { "someDeviceId" } else { "" };

                oneshot_send_and_log(
                    msg.callback,
                    DeviceResponseMessage::call(json!({"success" : true, "xDeviceId": r })),
                    "",
                );
            }),
        );

        ch.custom_request_handler.insert(
            "org.rdk.AuthService.getServiceAccountId".to_owned(),
            Arc::new(|msg: ThunderCallMessage| {
                let result = { LOCAL_TEST_ID.load(Ordering::Relaxed) > 0 };
                let r = if result { "someAccountId" } else { "" };

                oneshot_send_and_log(
                    msg.callback,
                    DeviceResponseMessage::call(json!({"success" : true, "serviceAccountId": r })),
                    "",
                );
            }),
        );

        ch.custom_request_handler.insert(
            "org.rdk.AuthService.getDeviceId".to_owned(),
            Arc::new(|msg: ThunderCallMessage| {
                let result = { LOCAL_TEST_ID.load(Ordering::Relaxed) > 0 };

                let r = if result { "somePartnerId" } else { "" };

                oneshot_send_and_log(
                    msg.callback,
                    DeviceResponseMessage::call(json!({"success" : true, "partnerId": r })),
                    "",
                );
            }),
        );

        ch.custom_request_handler.insert(
            "org.rdk.AuthService.getServiceAccessToken".to_owned(),
            Arc::new(|msg: ThunderCallMessage| {
                let result = { LOCAL_TEST_ID.load(Ordering::Relaxed) > 0 };

                let r = if result { "someToken" } else { "" };

                oneshot_send_and_log(
                    msg.callback,
                    DeviceResponseMessage::call(json!({"success" : true, "token": r })),
                    "",
                );
            }),
        );

        ch.custom_subscription_handler.insert(
            "org.rdk.AuthService.1.serviceAccessTokenChanged".to_owned(),
            MockThunderSubscriberfn::new(|sender| {
                Box::pin(async {
                    tokio::spawn(async move {
                        loop {
                            let result = { LOCAL_TEST_ID.load(Ordering::Relaxed) > 0 };
                            if result {
                                debug!("no_token_on_startup returning message");
                                let _ = sender
                                    .send(DeviceResponseMessage {
                                        message: json!({}),
                                        sub_id: None,
                                    })
                                    .await;
                                break;
                            } else {
                                tokio::time::sleep(Duration::from_millis(10)).await
                            }
                        }
                    });
                    None
                })
            }),
        );
        let thunder = MockThunderController::get_thunder_state_mock_with_handler(Some(ch));
        let distributor_state = DistributorState::new(
            &DeviceManifest::default(),
            thunder,
            AppsanityConfig::mock(),
            AuthService::new(ThorPermissionService::mock(), OttTokenService::mock()),
            None,
        );
        handle_session(distributor_state.clone()).await;
        assert!(distributor_state.get_account_session().is_none());
        increment();
        tokio::time::sleep(Duration::from_millis(20)).await;
        assert!(distributor_state.get_account_session().is_some());
    }

    // This is a time based test used to validate a proper back off
    #[tokio::test]
    async fn token_available_check_session_back_off() {
        let mut ch = CustomHandler::default();
        ch.custom_request_handler.insert(
            "org.rdk.AuthService.getXDeviceId".to_owned(),
            Arc::new(|msg: ThunderCallMessage| {
                oneshot_send_and_log(
                    msg.callback,
                    DeviceResponseMessage::call(json!({"success" : true, "xDeviceId": "" })),
                    "",
                );
            }),
        );

        ch.custom_request_handler.insert(
            "org.rdk.AuthService.getServiceAccountId".to_owned(),
            Arc::new(|msg: ThunderCallMessage| {
                oneshot_send_and_log(
                    msg.callback,
                    DeviceResponseMessage::call(json!({"success" : true, "serviceAccountId": "" })),
                    "",
                );
            }),
        );

        ch.custom_request_handler.insert(
            "org.rdk.AuthService.getDeviceId".to_owned(),
            Arc::new(|msg: ThunderCallMessage| {
                oneshot_send_and_log(
                    msg.callback,
                    DeviceResponseMessage::call(json!({"success" : true, "partnerId": "" })),
                    "",
                );
            }),
        );

        ch.custom_request_handler.insert(
            "org.rdk.AuthService.getServiceAccessToken".to_owned(),
            Arc::new(|msg: ThunderCallMessage| {
                oneshot_send_and_log(
                    msg.callback,
                    DeviceResponseMessage::call(json!({"success" : true, "token": "" })),
                    "",
                );
            }),
        );
        let thunder = MockThunderController::get_thunder_state_mock_with_handler(Some(ch));
        let distributor_state = DistributorState::new(
            &DeviceManifest::default(),
            thunder,
            AppsanityConfig::mock(),
            AuthService::new(ThorPermissionService::mock(), OttTokenService::mock()),
            None,
        );
        let total_back_off = token_available_check_session(distributor_state.clone()).await;
        assert!(total_back_off.is_err());
        if let Err(e) = total_back_off {
            assert!(e == 1440)
        }
    }
}
