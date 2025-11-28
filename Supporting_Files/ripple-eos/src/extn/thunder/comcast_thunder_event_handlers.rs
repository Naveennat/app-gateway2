use std::time::Duration;

use thunder_ripple_sdk::{
    client::device_operator::{DeviceCallRequest, DeviceOperator},
    events::thunder_event_processor::{
        ThunderEventHandler, ThunderEventHandlerProvider, ThunderEventMessage,
    },
    ripple_sdk::{
        api::{
            context::RippleContextUpdateRequest,
            device::{device_events::DeviceEventCallback, device_request::AccountToken},
        },
        extn::extn_client_message::ExtnEvent,
        log::error,
        serde_json, tokio,
        utils::error::RippleError,
    },
    thunder_state::ThunderState,
};

use crate::extn::thunder::thunder_session::ThunderSessionService;

pub fn is_custom_event(value: ThunderEventMessage) -> bool {
    if let ThunderEventMessage::Custom(_) = value {
        return true;
    }
    false
}

// -----------------------
// Session Token Changed
pub const SESSION_TOKEN_CHANGED_EVENT: &str = "account.onServiceAccessTokenChanged";
pub struct SessionTokenChangeHandler;
impl SessionTokenChangeHandler {
    pub fn handle(
        state: ThunderState,
        value: ThunderEventMessage,
        _callback_type: DeviceEventCallback,
    ) {
        tokio::spawn(async move {
            if let ThunderEventMessage::Custom(_input) = value {
                let mut wait: u64 = 1;
                let mut previous_back_off = 0;
                let mut current_back_off = 1;
                loop {
                    if ThunderSessionService::get_account_session(&state)
                        .await
                        .is_err()
                    {
                        let temp = previous_back_off;
                        previous_back_off = current_back_off;
                        current_back_off = temp + previous_back_off;

                        tokio::time::sleep(Duration::from_secs(wait)).await;
                        wait += 1;
                        if wait > 10 {
                            state
                                .event_processor
                                .set_backoff(SESSION_TOKEN_CHANGED_EVENT, current_back_off);
                            error!("Not able to retrieve account session after 10 retries");
                            break;
                        } else {
                            continue;
                        }
                    } else {
                        state
                            .event_processor
                            .clear_backoff(SESSION_TOKEN_CHANGED_EVENT);
                        break;
                    }
                }
                let dev_req = DeviceCallRequest {
                    method: "org.rdk.AuthService.1.getServiceAccessToken".to_owned(),
                    params: None,
                };
                let dev_response = state.get_thunder_client().call(dev_req).await;
                if let Some(_) = dev_response.message.get("token") {
                    let result =
                        serde_json::from_value::<AccountToken>(dev_response.message.to_owned());
                    if result.is_ok() {
                        let value = result.unwrap();
                        ThunderEventHandler::callback_context_update(
                            state,
                            RippleContextUpdateRequest::Token(value),
                        )
                    }
                }
            } else {
                error!("Not intended receiver");
            }
        });
    }
}

impl ThunderEventHandlerProvider for SessionTokenChangeHandler {
    type EVENT = AccountToken;
    fn provide(id: String, callback_type: DeviceEventCallback) -> ThunderEventHandler {
        ThunderEventHandler {
            request: Self::get_device_request(),
            handle: Self::handle,
            is_valid: is_custom_event,
            listeners: vec![id],
            id: Self::get_mapped_event(),
            callback_type,
        }
    }

    fn get_mapped_event() -> String {
        SESSION_TOKEN_CHANGED_EVENT.into()
    }

    fn event_name() -> String {
        "serviceAccessTokenChanged".into()
    }

    fn module() -> String {
        "org.rdk.AuthService.1".to_owned()
    }
    fn get_extn_event(
        _r: Self::EVENT,
        _callback_type: DeviceEventCallback,
    ) -> Result<ExtnEvent, RippleError> {
        Err(RippleError::InvalidOutput)
    }
}
