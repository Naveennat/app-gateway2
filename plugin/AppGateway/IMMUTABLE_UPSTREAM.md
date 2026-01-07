# IMMUTABLE: Upstream-synced AppGateway plugin sources

This directory **MUST remain byte-for-byte identical** to the upstream directory:

- Upstream repo: https://github.com/rdkcentral/entservices-infra
- Branch: `develop`
- Path: `AppGateway/`
- Synced commit: `fbfd9f55a7a9c5b2723f5f05f4d8d1cae4722dd9`

## Rules

1. **Do not edit** any files under `plugin/AppGateway/`, especially `*.cpp` and `*.h`.
2. All test-only changes MUST go under: `tests/l0/appgateway/**`
3. Any build/coverage/script/CMake adjustments MUST be done **outside** `plugin/AppGateway/` and must treat this directory as **read-only**.
4. If upstream changes are needed, re-sync by replacing this directory with the upstream contents from the repo/branch above, and update the commit hash in this file.

## Why

The user requirement for this work item is to keep `plugin/AppGateway` aligned with upstream `entservices-infra` develop branch and to ensure future prompts do not drift plugin sources.

(Backup of prior local version may exist as `plugin/AppGateway.local-backup` in this repo workspace.)
