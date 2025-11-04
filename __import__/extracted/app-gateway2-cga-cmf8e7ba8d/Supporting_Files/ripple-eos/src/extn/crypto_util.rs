use cryptoxide::{digest::Digest, sha1::Sha1};
use thunder_ripple_sdk::ripple_sdk::api::manifest::device_manifest::IdSalt;
use url::Url;
use uuid::Uuid;

pub fn uuid_from(r1: String, r2: String) -> String {
    let salt = format!("{}:{}", r1, r2);
    let uuid: Uuid = Uuid::new_v5(&Uuid::NAMESPACE_OID, &salt.as_bytes());
    uuid.hyphenated().to_string()
}

pub fn extract_app_ref(app_url: String) -> String {
    if let Ok(url) = Url::parse(&app_url.clone()) {
        if let Some(r) = url.domain() {
            let split_vec: Vec<&str> = r.split(".").collect();
            if split_vec.len() > 1 {
                let l = split_vec.len();
                return format!("{}.{}", split_vec[l - 2], split_vec[l - 1]);
            }
        }
    }
    app_url
}

/// Creates a uuid using a scope that is specific to an app
///
/// # Arguments
///
/// * `app_scope` - A string that makes this uuid scoped to an app, can be an id or a domain
/// * `reference` - The string that is the subject of the encryption
/// * `id_salt` - A constant salt
pub fn salt_using_app_scope(
    app_scope: String,
    reference: String,
    id_salt: Option<IdSalt>,
) -> String {
    if let Some(salt) = id_salt {
        let algo = salt.algorithm;
        let magic = salt.magic;
        if let Some(algorithm) = algo {
            if algorithm.eq("SHA1") {
                let mut hasher = Sha1::new();
                hasher.input_str(&app_scope);
                hasher.input_str(&reference);
                if let Some(m) = magic {
                    hasher.input_str(&m);
                    return hasher.result_str();
                } else {
                    return hasher.result_str();
                }
            }
        }
    }
    uuid_from(reference, app_scope)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn test_salting() {
        let domain = extract_app_ref(
            "https://firecertapp.firecert.comcast.com/prod/index.html?systemui=true".into(),
        );
        let r = salt_using_app_scope(
            domain,
            "1234567891011121314".into(),
            Some(IdSalt {
                algorithm: Some("SHA1".into()),
                magic: Some("5dA6Sg34jdG0PhXf".into()),
            }),
        );

        assert_eq!(r, String::from("fe8c502cb9c226a384654410706381c33dcaa0cb"));
    }
}
