use cryptoxide::{digest::Digest, sha1::Sha1};

/// Creates a uuid using a scope that is specific to an app
///
/// # Arguments
///
/// * `app_scope` - A string that makes this uuid scoped to an app, can be an id or a domain
/// * `reference` - The string that is the subject of the encryption
/// * `magic` - A constant
pub fn salt_using_app_scope(
    app_scope: String,
    reference: String,
    magic: Option<String>,
) -> Option<String> {
    let mut hasher = Sha1::new();
    hasher.input_str(&app_scope);
    hasher.input_str(&reference);
    if let Some(m) = magic {
        hasher.input_str(&m);
    }
    Some(hasher.result_str())
}
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_salt_using_app_scope_with_magic() {
        let app_scope = "app1".to_string();
        let reference = "ref1".to_string();
        let magic = Some("magic1".to_string());
        let result = salt_using_app_scope(app_scope, reference, magic);
        assert_eq!(
            result,
            Some("d02f1831f111b994659dfdd8c5885d3c6c23f1ba".to_string())
        );
    }

    #[test]
    fn test_salt_using_app_scope_without_magic() {
        let app_scope = "app1".to_string();
        let reference = "ref1".to_string();
        let result = salt_using_app_scope(app_scope, reference, None);
        assert_eq!(
            result,
            Some("9d87c56bfb8d7b7f2bb9138f20f1d4d25605ff48".to_string())
        );
    }

    #[test]
    fn test_salt_using_app_scope_empty_strings() {
        let app_scope = "".to_string();
        let reference = "".to_string();
        let result = salt_using_app_scope(app_scope, reference, None);
        assert_eq!(
            result,
            Some("da39a3ee5e6b4b0d3255bfef95601890afd80709".to_string())
        );
    }

    #[test]
    fn test_salt_using_app_scope_only_magic() {
        let app_scope = "".to_string();
        let reference = "".to_string();
        let magic = Some("magic1".to_string());
        let result = salt_using_app_scope(app_scope, reference, magic);
        assert_eq!(
            result,
            Some("c41b08fae98da2cfdb80447e9a96e84bcfd051b8".to_string())
        );
    }

    #[test]
    fn test_salt_using_app_scope_long_strings() {
        let app_scope = "a".repeat(1000);
        let reference = "b".repeat(1000);
        let magic = Some("c".repeat(1000));
        let result = salt_using_app_scope(app_scope, reference, magic);
        assert_eq!(
            result,
            Some("f15bbe1198d15d17ccb647c4f28ef862c62f2084".to_string())
        );
    }
}
