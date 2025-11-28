use std::fs;
use std::fs::read_to_string;
use std::path::PathBuf;
use toml;

pub fn get_cargo_registry_path() -> Option<PathBuf> {
    if let Some(cargo_home) = std::env::var("CARGO_HOME").ok() {
        let mut path = PathBuf::from(cargo_home);
        path.push("registry/src");
        Some(path)
    } else {
        None
    }
}

// get_crate_source_path: Given a registry and crate name, locates crate entry in Cargo.toml and returns the path to the crate source.
pub fn get_crate_source_path(registry: &str, crate_name: &str) -> Option<String> {
    let crate_source_path = match get_cargo_registry_path() {
        Some(path) => path,
        None => {
            println!("get_crate_source_path: Failed to get cargo registry path");
            return None;
        }
    };

    let cargo_toml_str = match read_to_string("Cargo.toml") {
        Ok(s) => s,
        Err(e) => {
            println!("get_crate_source_path: Could not read file: e={:?}", e);
            return None;
        }
    };

    let cargo_toml_value: toml::Value = match toml::from_str(&cargo_toml_str) {
        Ok(v) => v,
        Err(e) => {
            println!(
                "get_crate_source_path: Could not parse Cargo.toml: e={:?}",
                e
            );
            return None;
        }
    };

    let cargo_toml_table = match cargo_toml_value.as_table() {
        Some(t) => t,
        None => {
            println!("get_crate_source_path: Unexpected file format");
            return None;
        }
    };

    let dependencies_table = match cargo_toml_table.get("dependencies") {
        Some(t) => t,
        None => {
            println!("get_crate_source_path: Missing dependencies section");
            return None;
        }
    };

    let crate_table = match dependencies_table.get(crate_name) {
        Some(t) => t,
        None => {
            println!("get_crate_source_path: Missing crate in dependencies section");
            return None;
        }
    };

    let crate_version = match crate_table.get("version") {
        Some(t) => {
            if let Some(v) = t.as_str() {
                v
            } else {
                println!("get_crate_source_path: Unexpected version format");
                return None;
            }
        }
        None => {
            println!("get_crate_source_path: Missing version in crate section");
            return None;
        }
    };

    let entries = match fs::read_dir(crate_source_path.clone()) {
        Ok(e) => e,
        Err(e) => {
            println!("get_crate_source_path: Could not read directory: e={:?}", e);
            return None;
        }
    };

    for entry in entries {
        if let Ok(entry) = entry {
            let path = entry.path();
            if path.is_dir() {
                let dir = path.file_name().unwrap().to_string_lossy().into_owned();
                if dir.starts_with(registry) {
                    let crate_source_path_str =
                        crate_source_path.join(path).to_string_lossy().into_owned();
                    return Some(format!(
                        "{}/{}-{}",
                        crate_source_path_str, crate_name, crate_version
                    ));
                }
            }
        }
    }
    println!("get_crate_source_path: Crate source path not found");
    None
}
