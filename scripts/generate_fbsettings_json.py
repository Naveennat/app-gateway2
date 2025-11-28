#!/usr/bin/env python3
"""
Generate a sample FbSettings JSON file for the app-gateway2/app-gateway/FbSettings plugin.

This utility ensures the output directory exists, optionally reads the SystemSettingRules,
and writes a sample JSON payload that can be used for validation, tests, or demos.
"""

import argparse
import json
import os
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional, Dict, Any


DEFAULT_FILENAME = "sample_settings.json"


def _repo_root_from_script(script_path: Path) -> Path:
    """
    Resolve project root based on the known structure:
    <repo-root>/app-gateway2/scripts/generate_fbsettings_json.py
    """
    # scripts_dir is app-gateway2/scripts
    scripts_dir = script_path.parent
    # repo_root: go one level up from app-gateway2
    return scripts_dir.parent


def _default_output_dir(repo_root: Path) -> Path:
    """
    Provide default output directory within the project:
    <repo-root>/app-gateway2/app-gateway/FbSettings/output
    """
    return repo_root / "app-gateway2" / "app-gateway" / "FbSettings" / "output"


def _default_rules_path(repo_root: Path) -> Path:
    """
    Provide default rules file path:
    <repo-root>/app-gateway2/app-gateway/FbSettings/SystemSettingRules/system_settings_rules.json
    """
    return repo_root / "app-gateway2" / "app-gateway" / "FbSettings" / "SystemSettingRules" / "system_settings_rules.json"


def _load_rules(rules_path: Path) -> Optional[Dict[str, Any]]:
    """Attempt to load the rules JSON; return dict or None on failure."""
    try:
        if rules_path.exists():
            with rules_path.open("r", encoding="utf-8") as fh:
                return json.load(fh)
        return None
    except Exception as e:
        print(f"[WARN] Failed to load rules from {rules_path}: {e}", file=sys.stderr)
        return None


def _ensure_dir(path: Path) -> None:
    """Create directory if it does not exist."""
    try:
        path.mkdir(parents=True, exist_ok=True)
    except Exception as e:
        print(f"[ERROR] Unable to create directory: {path} -> {e}", file=sys.stderr)
        raise


# PUBLIC_INTERFACE
def generate_sample_fbsettings_json(output_dir: Path, filename: str = DEFAULT_FILENAME, rules_path: Optional[Path] = None, include_rules: bool = False) -> Path:
    """
    Generate a sample FbSettings JSON file to the specified output directory.

    Parameters:
        output_dir (Path): The directory where the JSON file should be written.
        filename (str): The output file name (default: 'sample_settings.json').
        rules_path (Optional[Path]): Path to system_settings_rules.json if known. If provided and exists, it will be referenced in metadata, and optionally embedded.
        include_rules (bool): If True and rules_path is valid, embed the rules content into the output JSON.

    Returns:
        Path: Full path to the generated JSON file.
    """
    # Resolve and prepare output
    output_dir = output_dir.resolve()
    _ensure_dir(output_dir)

    # Load rules if requested
    rules_obj = None
    if rules_path is not None:
        rules_path = rules_path.resolve()
        rules_obj = _load_rules(rules_path)

    # Prepare sample payload
    now = datetime.now(timezone.utc).isoformat()
    payload: Dict[str, Any] = {
        "meta": {
            "generated_at": now,
            "generator": "scripts/generate_fbsettings_json.py",
            "version": "1.0.0",
            "rules_path": str(rules_path) if rules_path else None
        },
        "settings": {
            "device": {
                "make": "unknown",
                "name": "Living Room",
                "sku": "SKUX1"
            },
            "localization": {
                "countryCode": "US",
                "timeZone": "America/New_York"
            },
            "secondscreen": {
                "friendlyName": "Living Room"
            }
        }
    }

    # Optionally embed rules
    if include_rules and rules_obj is not None:
        payload["source_rules"] = rules_obj

    # Serialize and write
    out_file = output_dir / filename
    try:
        with out_file.open("w", encoding="utf-8") as fh:
            json.dump(payload, fh, indent=2, ensure_ascii=False, sort_keys=False)
        print(f"[INFO] Wrote sample FbSettings JSON: {out_file}")
    except Exception as e:
        print(f"[ERROR] Failed to write JSON file {out_file}: {e}", file=sys.stderr)
        raise

    return out_file


# PUBLIC_INTERFACE
def main() -> int:
    """
    CLI entrypoint. Generates a sample FbSettings JSON file.

    Supported overrides:
      - The output directory can be overridden with --output-dir or the FBSETTINGS_OUTPUT_DIR environment variable.
      - The rules file path can be overridden with --rules-file; otherwise a default location is used.
      - The output filename can be changed with --file-name.
      - Use --include-rules to embed the rules JSON directly in the output.
    """
    parser = argparse.ArgumentParser(description="Generate a sample FbSettings JSON file under app-gateway/FbSettings/output.")
    parser.add_argument("--output-dir", type=str, default=None, help="Directory to write JSON output (defaults to app-gateway/FbSettings/output).")
    parser.add_argument("--file-name", type=str, default=DEFAULT_FILENAME, help=f"Output file name (default: {DEFAULT_FILENAME})")
    parser.add_argument("--rules-file", type=str, default=None, help="Path to system_settings_rules.json; uses project default if not provided.")
    parser.add_argument("--include-rules", action="store_true", help="Embed rules JSON in the output.")
    args = parser.parse_args()

    script_path = Path(__file__).resolve()
    repo_root = _repo_root_from_script(script_path)

    output_dir = (
        Path(args.output_dir).expanduser()
        if args.output_dir
        else Path(os.environ.get("FBSETTINGS_OUTPUT_DIR", ""))
        if os.environ.get("FBSETTINGS_OUTPUT_DIR")
        else _default_output_dir(repo_root)
    )

    rules_path = (
        Path(args.rules_file).expanduser()
        if args.rules_file
        else _default_rules_path(repo_root)
    )

    try:
        generate_sample_fbsettings_json(output_dir=output_dir, filename=args.file_name, rules_path=rules_path, include_rules=args.include_rules)
        return 0
    except Exception:
        return 1


if __name__ == "__main__":
    sys.exit(main())
