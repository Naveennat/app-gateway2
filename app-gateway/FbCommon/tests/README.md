# FbCommon curl tests

This folder contains curl-based scripts to exercise AppGateway JSON-RPC resolution for the org.rdk.FbCommon COM-RPC handler.

Usage:
- Copy env.example.sh to env.sh and adjust values if needed.
- Source env.sh or rely on defaults.
- Run any script, e.g.:
  ./device.make.get.sh
  ./device.name.get.sh
  ./device.setName.set.sh "Living Room"
  ./events.displaysettings.subscribe.sh on
  ./events.hdcp.subscribe.sh on

Notes:
- Scripts target AppGateway's JSON-RPC endpoint: http://$APPGATEWAY_HOST:$APPGATEWAY_PORT/jsonrpc
- Each payload is JSON-RPC 2.0 with the Firebolt method name (e.g., "device.audio").
- Event scripts send {"listen": true|false} for event register/unregister.
- Resolution is handled via AppGateway (useComRpc enabled) and routes to org.rdk.FbCommon.
