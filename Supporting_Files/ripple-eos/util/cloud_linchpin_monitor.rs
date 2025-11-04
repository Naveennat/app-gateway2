use std::collections::HashMap;

// #[cfg(feature = "thunder_linchpin_client")]
use super::cloud_sync_monitor_utils::{LinchpinProxyCommand, StateRequest};
use crate::gateway::appsanity_gateway::defaults;
use crate::util::cloud_sync_monitor_utils::LinchpinPayload;
use crate::util::{cloud_sync_monitor_utils::ConnectParam, sync_settings::SyncSettings};
use serde::{Deserialize, Serialize};
use serde_json::json;
use serde_json::Value;
use std::sync::{Arc, RwLock};
use thunder_ripple_sdk::ripple_sdk::api::device::device_info_request::DeviceInfoRequest;
use thunder_ripple_sdk::ripple_sdk::api::gateway::rpc_gateway_api::{
    JsonRpcApiResponse, RpcRequest,
};
use thunder_ripple_sdk::ripple_sdk::extn::client::extn_client::ExtnClient;
use thunder_ripple_sdk::ripple_sdk::extn::extn_client_message::{ExtnEvent, ExtnMessage};
use thunder_ripple_sdk::ripple_sdk::log::{debug, error};
use thunder_ripple_sdk::ripple_sdk::utils::error::RippleError;
use tokio::sync::oneshot::Sender as MSender;
use tokio::sync::{
    mpsc::{self, Sender},
    oneshot,
};
use tonic::async_trait;
use url::ParseError;

#[derive(Deserialize, Debug, Clone, PartialEq)]
pub enum LinchpinConnectionStatus {
    Disconnected,
    Connected,
    ConnectionPending,
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub enum PubSubConnectionStatus {
    Connected(String),
    Disconnected(String),
    Reconnecting(String),
}

#[derive(Debug, PartialEq, Serialize, Deserialize, Clone)]
pub struct PubSubNotifyTopic {
    pub connection_id: String,
    pub topic: String,
    pub value: Value,
}

#[derive(Debug, PartialEq)]
pub enum ClientState {
    // Initial state or state after a successful disconnection
    NotConnected,
    Connected,
    Reconnecting,
    Disconnected(LinchpinErrors),
    // First initial connection failure if it is unauthorized using a future ripple context impl
    // we can plan for retry after a token update
    ConnectionFailed(LinchpinErrors),
    ReconnectFailed(LinchpinErrors),
}

#[derive(Debug, PartialEq)]
pub enum LinchpinErrors {
    InvalidRequest,
    Disconnected,
    IoError,
    NotFound,
    Timeout,
    Unauthorized,
    LimitReached,
    Unknown,
    Unavailble,
    Reconnecting,
    AlreadySubscribed,
}

// #[cfg(feature = "thunder_linchpin_client")]
#[derive(Deserialize, Debug, Clone)]
pub enum LinchpinEvents {
    ConnectionEvent(LinchpinConnectionStatus),
    ValueChangeEvent(PubSubNotifyTopic),
}
// #[cfg(feature = "thunder_linchpin_client")]
#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct ValueChangePayload {
    pub header: Option<HashMap<String, String>>,
    pub payload: String,
    #[serde(rename = "payloadType")]
    pub payload_type: String,
    pub success: bool,
}

#[derive(Debug, Clone, Deserialize)]
#[cfg_attr(test, derive(PartialEq))]
#[serde(rename_all = "camelCase")]
pub struct NotifyMessage {
    pub request_id: String,
    pub headers: Option<HashMap<String, String>>,
    pub message_type: Option<String>,
    pub topic: String,
    pub payload: String,
    pub payload_type: String,
}

#[derive(Debug, Clone, PartialEq)]
pub enum LinchpinClientError {
    InvalidRequest,
    Disconnected,
    IoError,
    NotFound,
    Timeout,
    Unauthorized,
    LimitReached,
    Unknown,
    Unavailble,
    AlreadySubscribed,
}

impl From<ParseError> for LinchpinClientError {
    fn from(_: ParseError) -> Self {
        LinchpinClientError::InvalidRequest
    }
}

#[derive(Deserialize, Debug, Clone)]
pub struct LinchpinConfig {
    #[serde(default = "linchpin_config_url_default")]
    pub url: String,
    #[serde(default = "linchpin_config_initial_reconnect_delay_ms_default")]
    pub initial_reconnect_delay_ms: Option<u64>,
    #[serde(default = "linchpin_config_max_reconnect_delay_ms_default")]
    pub max_reconnect_delay_ms: Option<u64>,
    #[serde(default = "linchpin_config_heartbeat_timeout_secs_default")]
    pub heartbeat_timeout_secs: Option<u64>,
}

pub fn linchpin_config_url_default() -> String {
    defaults().linchpin_config.url.clone()
}

pub fn linchpin_config_initial_reconnect_delay_ms_default() -> Option<u64> {
    defaults().linchpin_config.initial_reconnect_delay_ms
}

pub fn linchpin_config_max_reconnect_delay_ms_default() -> Option<u64> {
    defaults().linchpin_config.max_reconnect_delay_ms
}

pub fn linchpin_config_heartbeat_timeout_secs_default() -> Option<u64> {
    defaults().linchpin_config.heartbeat_timeout_secs
}

#[derive(Debug, Clone)]
pub struct CloudLinchpinMonitor {
    command_tx: Sender<LinchpinProxyCommand>,
    state_tx: Sender<StateRequest>,
}

impl CloudLinchpinMonitor {
    pub fn start(extn_client: ExtnClient, state_tx: Sender<StateRequest>) -> Self {
        let (client_event_tx, mut client_event_rx) = mpsc::channel::<LinchpinEvents>(32);
        let (command_tx, mut command_rx) = mpsc::channel::<LinchpinProxyCommand>(32);
        let extn_client_c = extn_client.clone();
        let client_event_tx_c = client_event_tx.clone();
        let linchpin_handler = async move {
            let mut as_linchpin_client =
                ASLinchpinClient::new(extn_client_c, client_event_tx_c.clone());

            while let Some(message) = command_rx.recv().await {
                match message {
                    LinchpinProxyCommand::Connect(connect_param) => {
                        if as_linchpin_client.is_connected() {
                            continue;
                        }
                        if as_linchpin_client.connect(connect_param).await.is_err() {
                            error!("external linchpin connection error");
                        } else {
                            as_linchpin_client.handle_connected();
                        }
                    }
                    LinchpinProxyCommand::Subscribe(topic, callback) => {
                        as_linchpin_client.subscribe(topic, callback).await;
                    }
                    LinchpinProxyCommand::Unsubscribe(topic, callback) => {
                        as_linchpin_client.unsubscribe(topic, callback).await;
                    }
                }
            }
        };

        tokio::spawn(linchpin_handler);

        let state_tx_c = state_tx.clone();
        let cmd_tx_c = command_tx.clone();
        tokio::spawn(async move {
            while let Some(client_event) = client_event_rx.recv().await {
                let state_tx_clone = state_tx_c.clone();
                let cmd_tx_clone = cmd_tx_c.clone();
                Self::process_linchpin_notification(state_tx_clone, cmd_tx_clone, &client_event)
                    .await;
            }
        });
        CloudLinchpinMonitor {
            command_tx,
            state_tx,
        }
    }

    pub async fn subscribe(
        &self,
        settings: SyncSettings,
        config: LinchpinConfig,
        dev_id: &str,
        sat: &str,
    ) {
        let listen_topic = settings.cloud_monitor_topic.to_owned();
        debug!("linchpin subscribe topic: {listen_topic}");
        debug!("Adding topic to pending: {listen_topic}");
        let _ = self
            .state_tx
            .send(StateRequest::AddPendingTopic(listen_topic))
            .await;
        let (callback_lp_connected_tx, callback_lp_connected_rx) = oneshot::channel();
        let _ = self
            .state_tx
            .send(StateRequest::GetLinchpinConnectionStatus(
                callback_lp_connected_tx,
            ))
            .await;
        if let Ok(connection_status) = callback_lp_connected_rx.await {
            debug!("linchpin connected status: {:?}", connection_status);
            match connection_status {
                LinchpinConnectionStatus::Disconnected => {
                    let connect_param = ConnectParam {
                        dev_id: dev_id.to_owned(),
                        sat: sat.to_owned(),
                        config,
                    };
                    let _ = self
                        .command_tx
                        .send(LinchpinProxyCommand::Connect(connect_param))
                        .await;
                }
                LinchpinConnectionStatus::Connected => {
                    Self::complete_subscribe(self.state_tx.clone(), self.command_tx.clone()).await;
                }
                LinchpinConnectionStatus::ConnectionPending => {
                    debug!("Pending linchpin connection. Waiting for connection to complete");
                }
            }
            debug!("Adding listner :{:?}", settings);
            let _ = self
                .state_tx
                .send(StateRequest::AddListener(settings))
                .await;
        } else {
            error!("Unable to receive linchpin connection status");
        }
    }

    async fn complete_subscribe(
        state_tx: Sender<StateRequest>,
        cmd_tx: Sender<LinchpinProxyCommand>,
    ) {
        loop {
            let state_tx_c = state_tx.clone();
            let cmd_tx_c = cmd_tx.clone();
            let all_subscribed = Self::subscribe_pending_topics(state_tx_c, cmd_tx_c).await;
            if all_subscribed {
                debug!("All topic subscribed");
                break;
            }
            tokio::time::sleep(tokio::time::Duration::from_secs(60)).await;
            debug!("retrying to subscribe remaining topic");
        }
    }
    async fn subscribe_pending_topics(
        state_tx: Sender<StateRequest>,
        cmd_tx: Sender<LinchpinProxyCommand>,
    ) -> bool {
        let mut all_subscribed = true;
        let (callback_pending_topic_tx, callback_pending_topic_rx) = oneshot::channel();
        let get_topic_res = state_tx
            .send(StateRequest::GetAllPendingTopics(callback_pending_topic_tx))
            .await;
        debug!(
            "response for get all pending subscribe topics: {:?}",
            get_topic_res
        );
        if let Ok(pending_list) = callback_pending_topic_rx.await {
            for pending_topic in pending_list {
                debug!("Sending subscribe event for topic: {:?}", pending_topic);
                let (response_tx, response_rx) = oneshot::channel::<Result<(), LinchpinErrors>>();
                let sub_res = cmd_tx
                    .clone()
                    .send(LinchpinProxyCommand::Subscribe(
                        pending_topic.to_owned(),
                        response_tx,
                    ))
                    .await;
                if sub_res.is_ok() {
                    if let Ok(res) = response_rx.await {
                        match res {
                            Ok(_) | Err(LinchpinErrors::AlreadySubscribed) => {
                                debug!("successfully subscribed to topic: {}", pending_topic);
                                let _ = state_tx
                                    .send(StateRequest::ClearPendingTopics(
                                        pending_topic.to_owned(),
                                    ))
                                    .await;
                            }
                            _ => {
                                debug!("Failed to subscribed to topic: {}", pending_topic);
                                all_subscribed = false;
                            }
                        }
                    }
                } else {
                    all_subscribed = false;
                }
            }
        } else {
            error!("unable to get pending subscribe topic");
            all_subscribed = false;
        }
        all_subscribed
    }

    pub async fn process_linchpin_notification(
        state_tx: Sender<StateRequest>,
        cmd_tx: Sender<LinchpinProxyCommand>,
        client_event: &LinchpinEvents,
    ) {
        debug!("about to linchpin process event: {:?}", client_event);
        match client_event {
            LinchpinEvents::ConnectionEvent(client_state) => {
                let _ = state_tx
                    .send(StateRequest::SetLinchpinConnectionStatus(
                        client_state.clone(),
                    ))
                    .await;
                if client_state == &LinchpinConnectionStatus::Connected {
                    Self::complete_subscribe(state_tx.clone(), cmd_tx.clone()).await;
                }
            }
            LinchpinEvents::ValueChangeEvent(message) => {
                let rx = match handle_linchpin_message(message, &state_tx).await {
                    Some(value) => value,
                    None => return,
                };
                if let Ok(listeners) = rx.await {
                    for listener in listeners {
                        let response = listener.get_values_from_cloud(state_tx.clone()).await;
                        if let Ok(resp) = response {
                            debug!("Sending dpabresponse to callback: {:?}", resp);
                            let _ = listener.callback.send(resp).await;
                        }
                    }
                }
            }
        }
    }
}

pub async fn handle_linchpin_message(
    message: &PubSubNotifyTopic,
    state_tx: &Sender<StateRequest>,
) -> Option<oneshot::Receiver<Vec<SyncSettings>>> {
    let topic = message.topic.as_str();
    debug!("Receivde linchpin event :{:?}", message);
    let res_linchpin_payload = serde_json::from_value::<ValueChangePayload>(message.value.clone());
    debug!("Received Linchpin Payload: {:?}", res_linchpin_payload);
    if let Err(e) = res_linchpin_payload {
        error!(
            "Unable to parse received payload into ValueChangePayload: {:?}, {:?}",
            message.value, e
        );
        return None;
    }
    let payload = serde_json::from_str::<LinchpinPayload>(&res_linchpin_payload.unwrap().payload);
    if let Err(e) = payload {
        error!(
            "Unable to parse received payload into LinchpinPayload: {:?}",
            e
        );
        return None;
    }
    let linchpin_payload = payload.unwrap();
    let updated_settings = linchpin_payload
        .event_payload
        .settings
        .as_object()
        .unwrap()
        .keys()
        .cloned()
        .collect();
    let (tx, rx) = oneshot::channel::<Vec<SyncSettings>>();
    let _ = state_tx
        .send(StateRequest::GetListenersForProperties(
            topic.to_owned(),
            updated_settings,
            tx,
        ))
        .await;
    Some(rx)
}

pub struct ASLinchpinClient {
    extn_client: ExtnClient,
    client_event_tx: Sender<LinchpinEvents>,
    connected: Arc<RwLock<Option<bool>>>,
}

#[derive(Deserialize, Debug, Clone)]
struct ASLinchpinPayload {
    connectionstate: ASConnectionState,
    #[serde(rename = "documentId")]
    document_id: Option<String>,
    notifications: Option<Vec<ASLinchpinNotification>>,
    topics: Option<Vec<ASLinchpinTopic>>,
}

#[derive(Debug, Deserialize, Clone)]
struct ASLinchpinNotification {
    payload: String, // This is a JSON string that needs further parsing
    #[serde(rename = "payloadType")]
    payload_type: String,
    topic: String,
}

#[derive(Debug, Deserialize, Clone)]
struct ASLinchpinTopic {
    status: String,
    topic: String,
}

#[derive(Deserialize, Debug, Clone)]
pub struct ASConnectionState {
    pub state: String,
    pub reason: Option<String>,
}

impl ASConnectionState {
    fn is_internet_check_required(&self) -> bool {
        if !self.is_connected() {
            if let Some(v) = &self.reason {
                return v.eq("heartbeat");
            }
        }

        false
    }

    fn is_connected(&self) -> bool {
        self.state.eq("connected")
    }
}

#[derive(Deserialize, Debug, Clone)]
pub struct ASLinchpinEvent {
    pub connectionstate: ASConnectionState,
    pub topic: String,
    pub payload: String,
}

impl From<ASLinchpinEvent> for ValueChangePayload {
    fn from(value: ASLinchpinEvent) -> Self {
        ValueChangePayload {
            header: None,
            payload: value.payload,
            success: true,
            payload_type: value.topic,
        }
    }
}

impl From<ASLinchpinNotification> for ValueChangePayload {
    fn from(value: ASLinchpinNotification) -> Self {
        ValueChangePayload {
            header: None,
            payload: value.payload,
            payload_type: value.payload_type,
            success: true,
        }
    }
}

impl From<ClientState> for LinchpinConnectionStatus {
    fn from(value: ClientState) -> Self {
        match value {
            ClientState::Connected => LinchpinConnectionStatus::Connected,
            ClientState::Reconnecting => LinchpinConnectionStatus::ConnectionPending,
            ClientState::Disconnected(_)
            | ClientState::NotConnected
            | ClientState::ConnectionFailed(_)
            | ClientState::ReconnectFailed(_) => LinchpinConnectionStatus::Disconnected,
        }
    }
}

impl From<PubSubConnectionStatus> for LinchpinConnectionStatus {
    fn from(value: PubSubConnectionStatus) -> Self {
        match value {
            PubSubConnectionStatus::Connected(_) => LinchpinConnectionStatus::Connected,
            PubSubConnectionStatus::Disconnected(_) => LinchpinConnectionStatus::Disconnected,
            PubSubConnectionStatus::Reconnecting(_) => LinchpinConnectionStatus::ConnectionPending,
        }
    }
}
impl From<NotifyMessage> for ValueChangePayload {
    fn from(value: NotifyMessage) -> Self {
        ValueChangePayload {
            header: value.headers.clone(),
            payload: value.payload.clone(),
            payload_type: value.payload_type.clone(),
            success: true,
        }
    }
}

impl From<LinchpinClientError> for LinchpinErrors {
    fn from(value: LinchpinClientError) -> Self {
        match value {
            LinchpinClientError::InvalidRequest => LinchpinErrors::InvalidRequest,
            LinchpinClientError::Disconnected => LinchpinErrors::Disconnected,
            LinchpinClientError::IoError => LinchpinErrors::IoError,
            LinchpinClientError::NotFound => LinchpinErrors::NotFound,
            LinchpinClientError::Timeout => LinchpinErrors::Timeout,
            LinchpinClientError::Unauthorized => LinchpinErrors::Unauthorized,
            LinchpinClientError::LimitReached => LinchpinErrors::LimitReached,
            LinchpinClientError::Unknown => LinchpinErrors::Unknown,
            LinchpinClientError::Unavailble => LinchpinErrors::Unavailble,
            LinchpinClientError::AlreadySubscribed => LinchpinErrors::AlreadySubscribed,
        }
    }
}

impl ASLinchpinClient {
    pub fn new(extn_client: ExtnClient, client_event_tx: Sender<LinchpinEvents>) -> Self {
        ASLinchpinClient {
            extn_client,
            client_event_tx,
            connected: Arc::new(RwLock::new(None)),
        }
    }

    pub fn handle_events(&self) -> Sender<ExtnMessage> {
        let (tx, mut tr) = mpsc::channel::<ExtnMessage>(10);
        let sender = self.client_event_tx.clone();
        let mut last_known_internet_value = true;
        let client = self.extn_client.clone();
        tokio::spawn(async move {
            while let Some(message) = tr.recv().await {
                if let Some(ExtnEvent::Value(v)) = message.payload.extract() {
                    if let Ok(json_rpc_api_message) =
                        serde_json::from_value::<JsonRpcApiResponse>(v)
                    {
                        if let Some(json_result) = json_rpc_api_message.result {
                            match serde_json::from_value::<ASLinchpinPayload>(json_result.clone()) {
                                Ok(v) => {
                                    // Process notifications if they exist
                                    if let Some(notifications) = &v.notifications {
                                        if !notifications.is_empty() {
                                            for notification in notifications {
                                                let topic = notification.topic.clone();
                                                let value: ValueChangePayload =
                                                    notification.clone().into();
                                                let pubsub_notify = PubSubNotifyTopic {
                                                    connection_id: topic.clone(),
                                                    topic,
                                                    value: serde_json::to_value(value).unwrap(),
                                                };
                                                if let Err(_) = sender.try_send(
                                                    LinchpinEvents::ValueChangeEvent(pubsub_notify),
                                                ) {
                                                    error!("Unable to forward Linchpin Notifications from AS")
                                                }
                                            }
                                        } else {
                                            debug!("Notifications array is empty");
                                        }
                                    } else {
                                        debug!("No notifications field in message");
                                    }

                                    // Handle connection state checking
                                    if !v.connectionstate.is_internet_check_required() {
                                        last_known_internet_value = false;
                                        let _ = client.request_transient(
                                            DeviceInfoRequest::InternetConnectionStatus,
                                        );
                                    } else if !last_known_internet_value
                                        && v.connectionstate.is_connected()
                                    {
                                        last_known_internet_value = true;
                                        let _ = client.request_transient(
                                            DeviceInfoRequest::InternetConnectionStatus,
                                        );
                                    }
                                }
                                Err(e) => {
                                    error!(
                                        "Failed to deserialize ASLinchpinPayload: {:?}. Raw JSON: {}",
                                        e,
                                        serde_json::to_string_pretty(&json_result)
                                            .unwrap_or_else(|_| format!("{:?}", json_result))
                                    );
                                }
                            }
                        } else {
                            error!("Invalid result - no result field");
                        }
                    } else {
                        error!("Invalid Json rpc api message");
                    }
                } else {
                    error!("Invalid message - not a Value event");
                }
            }
            error!("AS Linchpin connection backbone errored out");
        });
        tx
    }

    pub fn is_connected(&self) -> bool {
        self.connected.read().unwrap().clone().unwrap_or(false)
    }

    pub fn handle_connected(&self) {
        let mut connected = self.connected.write().unwrap();
        let _ = connected.insert(true);
    }
}
#[async_trait]
trait LinchpinClientRequestHandler {
    async fn connect(&mut self, connect_param: ConnectParam) -> Result<String, RippleError>;
    async fn subscribe(&mut self, topic: String, callback: MSender<Result<(), LinchpinErrors>>);
    async fn unsubscribe(&mut self, topic: String, callback: MSender<Result<(), LinchpinErrors>>);
}

#[async_trait]
impl LinchpinClientRequestHandler for ASLinchpinClient {
    async fn connect(&mut self, _: ConnectParam) -> Result<String, RippleError> {
        self.extn_client
            .subscribe(
                RpcRequest::get_new_internal(
                    "linchpin.onConnect".to_owned(),
                    Some(json!({"listen" : true})),
                ),
                self.handle_events(),
            )
            .await
    }
    async fn subscribe(&mut self, _: String, _: MSender<Result<(), LinchpinErrors>>) {
        // already done by AS
    }
    async fn unsubscribe(&mut self, _: String, _: MSender<Result<(), LinchpinErrors>>) {
        // already done by AS
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    #[test]
    fn test_deserialize_payload_with_notifications() {
        // JSON with notifications (from your logs)
        let json_with_notifications = json!({
            "connectionstate": {
                "state": "connected"
            },
            "documentId": "1750108507679",
            "notifications": [
                {
                    "payload": "{\"event_payload\":{\"environment\":\"prod\",\"settings\":{\"xcal:appContentAdTargeting\":{\"changeType\":\"updated\",\"ownerReference\":\"xrn:xcal:subscriber:account:4804527361917694294\",\"updated\":\"2025-06-16T21:15:06.486Z\",\"allowed\":true}}},\"timestamp\":1750108506488,\"event_schema\":\"xvp-ess-api/privacy-setting-state-change/1\",\"event_id\":\"9abd2c06-3019-46da-8d77-df152a5e699a\",\"account_id\":\"4804527361917694294\",\"partner_id\":\"xglobal\",\"source\":\"xvp-ess-api\"}",
                    "payloadType": "8200",
                    "topic": "p:xglobal:a:4804527361917694294:t:xvp-client"
                }
            ],
            "topics": [
                {
                    "status": "subscribed",
                    "topic": "p:xglobal:a:4804527361917694294:t:subscriber_change"
                },
                {
                    "status": "subscribed",
                    "topic": "p:xglobal:a:4804527361917694294:t:xvp-client"
                }
            ]
        });

        // Test deserialization
        let result = serde_json::from_value::<ASLinchpinPayload>(json_with_notifications);

        assert!(
            result.is_ok(),
            "Failed to deserialize JSON with notifications: {:?}",
            result.err()
        );

        let payload = result.unwrap();

        // Verify connection state
        assert_eq!(payload.connectionstate.state, "connected");
        assert!(payload.connectionstate.reason.is_none());

        // Verify document ID
        assert_eq!(payload.document_id, Some("1750108507679".to_string()));

        // Verify notifications
        assert!(payload.notifications.is_some());
        let notifications = payload.notifications.unwrap();
        assert_eq!(notifications.len(), 1);

        let notification = &notifications[0];
        assert_eq!(notification.payload_type, "8200");
        assert_eq!(
            notification.topic,
            "p:xglobal:a:4804527361917694294:t:xvp-client"
        );
        assert!(notification.payload.contains("xcal:appContentAdTargeting"));

        // Verify topics
        assert!(payload.topics.is_some());
        let topics = payload.topics.unwrap();
        assert_eq!(topics.len(), 2);
        assert_eq!(topics[0].status, "subscribed");
        assert_eq!(
            topics[0].topic,
            "p:xglobal:a:4804527361917694294:t:subscriber_change"
        );
    }

    #[test]
    fn test_deserialize_payload_without_notifications() {
        // JSON without notifications (from your other logs)
        let json_without_notifications = json!({
            "connectionstate": {
                "state": "connected"
            },
            "documentId": "1751042458969",
            "topics": [
                {
                    "status": "subscribed",
                    "topic": "p:xglobal:a:4804527361917694294:t:subscriber_change"
                },
                {
                    "status": "subscribed",
                    "topic": "p:xglobal:a:4804527361917694294:t:xvp-client"
                }
            ]
        });

        // Test deserialization
        let result = serde_json::from_value::<ASLinchpinPayload>(json_without_notifications);

        assert!(
            result.is_ok(),
            "Failed to deserialize JSON without notifications: {:?}",
            result.err()
        );

        let payload = result.unwrap();

        // Verify connection state
        assert_eq!(payload.connectionstate.state, "connected");
        assert!(payload.connectionstate.reason.is_none());

        // Verify document ID
        assert_eq!(payload.document_id, Some("1751042458969".to_string()));

        // Verify notifications (should be None)
        assert!(payload.notifications.is_none());

        // Verify topics
        assert!(payload.topics.is_some());
        let topics = payload.topics.unwrap();
        assert_eq!(topics.len(), 2);
        assert_eq!(topics[0].status, "subscribed");
        assert_eq!(
            topics[0].topic,
            "p:xglobal:a:4804527361917694294:t:subscriber_change"
        );
    }
}
