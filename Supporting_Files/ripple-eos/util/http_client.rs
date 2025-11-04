use hyper::header::CONTENT_TYPE;
use hyper::http::{HeaderValue, Request};
use hyper::{Body, Client, Method};
use hyper_tls::HttpsConnector;

use thunder_ripple_sdk::ripple_sdk::log::{debug, error};
use tower::{Service, ServiceBuilder, ServiceExt};
use tower_http::{
    auth::AddAuthorizationLayer, classify::StatusInRangeAsFailures,
    decompression::DecompressionLayer, set_header::SetRequestHeaderLayer, trace::TraceLayer,
};
use url::ParseError;

pub struct HttpClient {
    token: String,
    error_body: String,
}

impl HttpClient {
    pub fn new() -> Self {
        HttpClient {
            token: String::default(),
            error_body: String::default(),
        }
    }

    pub async fn post(&mut self, url: String, body: String) -> Result<String, HttpError> {
        self.execute(Method::POST, url, body).await
    }

    pub async fn get(&mut self, url: String, body: String) -> Result<String, HttpError> {
        self.execute(Method::GET, url, body).await
    }

    pub async fn put(&mut self, url: String, body: String) -> Result<String, HttpError> {
        self.execute(Method::PUT, url, body).await
    }

    pub async fn delete(&mut self, url: String, body: String) -> Result<String, HttpError> {
        self.execute(Method::DELETE, url, body).await
    }

    async fn execute(
        &mut self,
        method: Method,
        url: String,
        body: String,
    ) -> Result<String, HttpError> {
        let hyper_client = Client::builder().build::<_, hyper::Body>(HttpsConnector::new());

        let mut client = ServiceBuilder::new()
            .layer(TraceLayer::new(
                StatusInRangeAsFailures::new(400..=599).into_make_classifier(),
            ))
            .layer(SetRequestHeaderLayer::overriding(
                CONTENT_TYPE,
                HeaderValue::from_static("application/json; charset=UTF-8"),
            ))
            .layer(AddAuthorizationLayer::bearer(&self.token))
            .layer(DecompressionLayer::new())
            .service(hyper_client);

        let req = Request::builder()
            .uri(url.clone())
            .method(method.clone())
            .body(Body::from(body.clone()));

        debug!("execute: {:?}", req);

        if let Err(_) = req {
            error!("Could not compose request");
            return Err(HttpError::ParseError);
        }

        let request = req.unwrap();

        // TODO: What's the default response timeout for tower? Do we need to configure?
        let send_response = client.ready().await.unwrap().call(request).await;
        if let Err(e) = send_response {
            error!("error sending to Server={:?}", e);
            return Err(HttpError::ServiceError);
        }
        let response = send_response.unwrap();
        let status = response.status();
        debug!("status={} {url}", status);

        if response.status().is_success() {
            if let Ok(body_bytes) = hyper::body::to_bytes(response.into_body()).await {
                if let Ok(body_string) = String::from_utf8(body_bytes.to_vec()) {
                    return Ok(body_string);
                }
            }
        } else {
            error!("Request failed. status={:?}", status);
            if let Ok(body_bytes) = hyper::body::to_bytes(response.into_body()).await {
                if let Ok(body_string) = String::from_utf8(body_bytes.to_vec()) {
                    self.error_body = body_string.to_string();
                    error!("error_body={} ", self.error_body);
                }
            }

            return if status == 404 {
                Err(HttpError::NotDataFound)
            } else if status == 400 {
                Err(HttpError::BadRequestError)
            } else {
                Err(HttpError::ServiceError)
            };
        }
        Err(HttpError::ServiceError)
    }

    pub fn set_token(&mut self, token: String) -> &mut HttpClient {
        self.token = token;
        self
    }

    pub fn get_error_body(&self) -> String {
        self.error_body.clone()
    }
}

#[derive(Debug, Clone)]
pub enum HttpError {
    IoError,
    NotDataFound,
    ParseError,
    ServiceError,
    BadRequestError,
}

impl From<ParseError> for HttpError {
    fn from(_: ParseError) -> Self {
        HttpError::ParseError
    }
}

//impl From<http::Error> for HttpError {
//    fn from(_: http::Error) -> Self {
//        HttpError::IoError
//    }
//}

impl From<serde_json::Error> for HttpError {
    fn from(_: serde_json::Error) -> Self {
        HttpError::ParseError
    }
}

impl From<hyper::http::uri::InvalidUri> for HttpError {
    fn from(_: hyper::http::uri::InvalidUri) -> Self {
        HttpError::ParseError
    }
}
