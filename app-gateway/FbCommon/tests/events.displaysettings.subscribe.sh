#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/utils.sh"

# Usage: ./events.displaysettings.subscribe.sh [on|off] [event]
# event: resolutionChanged|audioFormatChanged (default: resolutionChanged)
STATE="${1:-on}"
EVENT="${2:-resolutionChanged}"

if [[ "${STATE}" == "on" || "${STATE}" == "true" ]]; then
  LISTEN="true"
else
  LISTEN="false"
fi

METHOD="displaySettings.${EVENT}"
event_toggle "${METHOD}" "${LISTEN}"
echo
