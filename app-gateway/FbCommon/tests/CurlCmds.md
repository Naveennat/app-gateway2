# FbCommon JSON-RPC cURL Commands

All examples use the default Thunder JSON-RPC endpoint at:
  http://127.0.0.1:9998/jsonrpc

Adjust host and/or port as needed for your environment.

## Controller: Activate / Deactivate org.rdk.FbCommon

These operations help you start/stop the FbCommon plugin before running the method tests.

- Activate FbCommon
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":4001,"method":"Controller.1.activate","params":{"callsign":"org.rdk.FbCommon"}}' \
  http://127.0.0.1:9998/jsonrpc
```

- Deactivate FbCommon
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":4002,"method":"Controller.1.deactivate","params":{"callsign":"org.rdk.FbCommon"}}' \
  http://127.0.0.1:9998/jsonrpc
```

---

## Firebolt methods resolved via AppGateway to org.rdk.FbCommon

Below are example requests for core device methods and events that are resolved by AppGateway to the org.rdk.FbCommon COM-RPC handler.  
- For getters/response-only methods, no params are included.
- For event subscriptions, the `listen` parameter is required.

### Device Methods

1) device.audio
- Tests retrieving current audio capabilities/settings (FbCommon provides structured or placeholder info)
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":4101,"method":"device.audio"}' \
  http://127.0.0.1:9998/jsonrpc
```
Example response:
```json
{"stereo":true,"dolbyAtmos":false,"dolbyDigital5.1":false,"dolbyDigital5.1+":false}
```

2) device.hdcp
- Tests retrieving current HDCP status via DisplaySettings/HdcpProfile delegation
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":4102,"method":"device.hdcp"}' \
  http://127.0.0.1:9998/jsonrpc
```

3) device.hdr
- Tests retrieving supported HDR modes (FbCommon returns structured or placeholder info)
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":4103,"method":"device.hdr"}' \
  http://127.0.0.1:9998/jsonrpc
```
Example response:
```json
{"hdr10":false,"dolbyVision":false,"hlg":false,"hdr10Plus":false}
```

4) device.screenResolution
- Tests retrieving current screen resolution via DisplaySettings delegation
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":4104,"method":"device.screenResolution"}' \
  http://127.0.0.1:9998/jsonrpc
```
Example response:
```json
[1920,1080]
```

5) device.videoResolution
- Tests retrieving current video resolution (proxied to screen resolution if not separately available)
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":4105,"method":"device.videoResolution"}' \
  http://127.0.0.1:9998/jsonrpc
```
Example response:
```json
[1920,1080]
```

---

### Device Events

The following are example subscribe/unsubscribe operations to be notified about device changes.
Pass `listen: true` to subscribe and `listen: false` to unsubscribe.

6) device.onScreenResolutionChanged (subscribe)
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":4201,"method":"device.onScreenResolutionChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

7) device.onVideoResolutionChanged (subscribe)
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":4202,"method":"device.onVideoResolutionChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

8) device.onAudioChanged (subscribe)
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":4203,"method":"device.onAudioChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

9) device.onHdrChanged (subscribe)
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":4204,"method":"device.onHdrChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

10) device.onHdcpChanged (subscribe)
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":4205,"method":"device.onHdcpChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

Notes:
- Event payloads and exact structures depend on the underlying DisplaySettings/HdcpProfile plugins.
- Ensure the AppGateway and relevant plugins are active before subscribing.
