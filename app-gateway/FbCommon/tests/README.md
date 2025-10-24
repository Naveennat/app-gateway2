# FbCommon curl tests

This folder contains curl-based scripts to exercise AppGateway JSON-RPC resolution for the org.rdk.FbCommon COM-RPC handler.

Quick start:
- Copy env.example.sh to env.sh and adjust values if needed.
- Source env.sh or rely on defaults.
- Ensure the plugin is active (see CurlCmds.md Controller examples), then run any script:
  ./device.audio.get.sh
  ./device.hdcp.get.sh
  ./device.hdr.get.sh
  ./device.screenResolution.get.sh
  ./device.videoResolution.get.sh
  ./events.displaysettings.subscribe.sh on
  ./events.hdcp.subscribe.sh on

Reference:
- For raw cURL examples equivalent to these scripts, see CurlCmds.md in this folder.
- Scripts target AppGateway's JSON-RPC endpoint: http://$APPGATEWAY_HOST:$APPGATEWAY_PORT/jsonrpc
- Each payload is JSON-RPC 2.0 with the Firebolt method name (e.g., "device.audio").
- Event scripts send {"listen": true|false} for event register/unregister.
- Resolution is handled via AppGateway (useComRpc enabled) and routes to org.rdk.FbCommon for the device methods documented here.
