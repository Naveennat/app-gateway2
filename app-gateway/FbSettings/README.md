# FbSettings JSON Generation

This folder contains the FbSettings plugin code and configuration. Historically, there was no script or build step to produce JSON artifacts here, which led to confusion about "generation failures." The plugin consumes JSON (e.g., system settings rules) but does not produce JSON by itself.

To enable easy validation and demo, a generator script is provided to create a sample JSON under `FbSettings/output`.

## Root cause

- No generator existed in the repository for FbSettings JSON outputs.
- The expected output directory did not exist (`app-gateway2/app-gateway/FbSettings/output`), so any attempt to write there would fail if not created first.
- There were no path or permission issues in the code because no write operation existed. The failure was effectively the lack of a generator and missing directory creation.

## What was added

- A robust Python CLI script with safe path handling and directory creation:
  `app-gateway2/scripts/generate_fbsettings_json.py`
- The script optionally reads the rules file to embed or reference it:
  `app-gateway2/app-gateway/FbSettings/SystemSettingRules/system_settings_rules.json`
- Clear logging and error handling are included.
- A sample output file can be generated on demand into `FbSettings/output/`.

## Requirements

- Python 3.7+ is recommended.
- Write permissions in the repository workspace.
- Optional: set environment variable `FBSETTINGS_OUTPUT_DIR` to override the default output directory.

## Usage

From the repository root:

```
python3 app-gateway2/scripts/generate_fbsettings_json.py
```

This will create:

```
app-gateway2/app-gateway/FbSettings/output/sample_settings.json
```

Options:

- Override output directory:
  ```
  python3 app-gateway2/scripts/generate_fbsettings_json.py --output-dir /path/to/output
  ```
  or set environment variable:
  ```
  FBSETTINGS_OUTPUT_DIR=/path/to/output python3 app-gateway2/scripts/generate_fbsettings_json.py
  ```

- Change output filename:
  ```
  python3 app-gateway2/scripts/generate_fbsettings_json.py --file-name my_settings.json
  ```

- Embed rules directly in the JSON:
  ```
  python3 app-gateway2/scripts/generate_fbsettings_json.py --include-rules
  ```

- Provide a custom rules file:
  ```
  python3 app-gateway2/scripts/generate_fbsettings_json.py --rules-file app-gateway2/app-gateway/FbSettings/SystemSettingRules/system_settings_rules.json
  ```

## Verification

- Ensure the following directory exists or let the script create it:
  `app-gateway2/app-gateway/FbSettings/output`
- Run:
  ```
  python3 app-gateway2/scripts/generate_fbsettings_json.py
  ```
- Confirm the output file is present:
  `app-gateway2/app-gateway/FbSettings/output/sample_settings.json`

## Notes

- The plugin does not write JSON artifacts during build; the generator is a standalone helper.
- The script resolves project-relative paths automatically and will not write outside the workspace unless explicitly instructed via command line or environment variables.
