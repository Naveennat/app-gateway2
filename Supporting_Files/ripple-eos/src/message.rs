use url::ParseError;
#[derive(Debug, PartialEq, Eq)]
pub enum Role {
    Use,
    Manage,
    Provide,
}

#[derive(Debug, Clone)]
pub struct PermissionServiceError {
    pub provider: String,
    pub message: String,
}
#[derive(Debug, Clone)]
pub enum DpabError {
    ServiceError,
    IoError,
    PermissionServiceError(PermissionServiceError),
    Exists,
    NotDataFound,
}

impl From<ParseError> for DpabError {
    fn from(_: ParseError) -> Self {
        DpabError::IoError
    }
}
