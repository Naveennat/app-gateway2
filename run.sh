#!/usr/bin/env bash
set -euo pipefail

# Wrapper entrypoint expected by certain preview systems.
# Delegates to the canonical scripts/start.sh
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${SCRIPT_DIR}/scripts/start.sh"
