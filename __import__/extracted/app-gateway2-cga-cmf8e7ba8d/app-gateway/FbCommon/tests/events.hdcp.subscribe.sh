#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/utils.sh"

# Usage: ./events.hdcp.subscribe.sh [on|off]
STATE="${1:-on}"
if [[ "${STATE}" == "on" || "${STATE}" == "true" ]]; then
  LISTEN="true"
else
  LISTEN="false"
fi

# Event name as used in resolutions for HdcpProfile
METHOD="HdcpProfile.onDisplayConnectionChanged"
event_toggle "${METHOD}" "${LISTEN}"
echo
