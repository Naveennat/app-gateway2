# FbPrivacy Thunder Plugin – cURL Test Commands


---

## Activate FbPrivacy

```bash
curl -d '{
  "jsonrpc": "2.0",
  "id": 122,
  "method": "Controller.1.activate",
  "params": { "callsign": "org.rdk.FbPrivacy" }
}' http://127.0.0.1:9998/jsonrpc
```

## Deactivate FbPrivacy

```bash
curl -d '{
  "jsonrpc": "2.0",
  "id": 122,
  "method": "Controller.1.deactivate",
  "params": { "callsign": "org.rdk.FbPrivacy" }
}' http://127.0.0.1:9998/jsonrpc
```

---

## Getter Methods

```bash
# Get allowACRCollection
curl -d '{"jsonrpc":"2.0","id":"1","method":"org.rdk.FbPrivacy.1.allowACRCollection"}' http://127.0.0.1:9998/jsonrpc

# Get allowAppContentAdTargeting
curl -d '{"jsonrpc":"2.0","id":"2","method":"org.rdk.FbPrivacy.1.allowAppContentAdTargeting"}' http://127.0.0.1:9998/jsonrpc

# Get allowCameraAnalytics
curl -d '{"jsonrpc":"2.0","id":"3","method":"org.rdk.FbPrivacy.1.allowCameraAnalytics"}' http://127.0.0.1:9998/jsonrpc

# Get allowPersonalization
curl -d '{"jsonrpc":"2.0","id":"4","method":"org.rdk.FbPrivacy.1.allowPersonalization"}' http://127.0.0.1:9998/jsonrpc

# Get allowPrimaryBrowseAdTargeting
curl -d '{"jsonrpc":"2.0","id":"5","method":"org.rdk.FbPrivacy.1.allowPrimaryBrowseAdTargeting"}' http://127.0.0.1:9998/jsonrpc

# Get allowPrimaryContentAdTargeting
curl -d '{"jsonrpc":"2.0","id":"6","method":"org.rdk.FbPrivacy.1.allowPrimaryContentAdTargeting"}' http://127.0.0.1:9998/jsonrpc

# Get allowProductAnalytics
curl -d '{"jsonrpc":"2.0","id":"7","method":"org.rdk.FbPrivacy.1.allowProductAnalytics"}' http://127.0.0.1:9998/jsonrpc

# Get allowRemoteDiagnostics
curl -d '{"jsonrpc":"2.0","id":"8","method":"org.rdk.FbPrivacy.1.allowRemoteDiagnostics"}' http://127.0.0.1:9998/jsonrpc

# Get allowResumePoints
curl -d '{"jsonrpc":"2.0","id":"9","method":"org.rdk.FbPrivacy.1.allowResumePoints"}' http://127.0.0.1:9998/jsonrpc

# Get allowUnentitledPersonalization
curl -d '{"jsonrpc":"2.0","id":"10","method":"org.rdk.FbPrivacy.1.allowUnentitledPersonalization"}' http://127.0.0.1:9998/jsonrpc

# Get allowUnentitledResumePoints
curl -d '{"jsonrpc":"2.0","id":"11","method":"org.rdk.FbPrivacy.1.allowUnentitledResumePoints"}' http://127.0.0.1:9998/jsonrpc

# Get allowWatchHistory
curl -d '{"jsonrpc":"2.0","id":"12","method":"org.rdk.FbPrivacy.1.allowWatchHistory"}' http://127.0.0.1:9998/jsonrpc

# Get all settings snapshot
curl -d '{"jsonrpc":"2.0","id":"13","method":"org.rdk.FbPrivacy.1.settings"}' http://127.0.0.1:9998/jsonrpc
```

---

## Setter Methods

Each setter requires a `"value": true|false` parameter.

```bash
# Set allowACRCollection
curl -d '{"jsonrpc":"2.0","id":"21","method":"org.rdk.FbPrivacy.1.setAllowACRCollection","params":{"value":true}}' http://127.0.0.1:9998/jsonrpc

# Set allowAppContentAdTargeting
curl -d '{"jsonrpc":"2.0","id":"22","method":"org.rdk.FbPrivacy.1.setAllowAppContentAdTargeting","params":{"value":false}}' http://127.0.0.1:9998/jsonrpc

# Set allowCameraAnalytics
curl -d '{"jsonrpc":"2.0","id":"23","method":"org.rdk.FbPrivacy.1.setAllowCameraAnalytics","params":{"value":true}}' http://127.0.0.1:9998/jsonrpc

# Set allowPersonalization
curl -d '{"jsonrpc":"2.0","id":"24","method":"org.rdk.FbPrivacy.1.setAllowPersonalization","params":{"value":true}}' http://127.0.0.1:9998/jsonrpc

# Set allowPrimaryBrowseAdTargeting
curl -d '{"jsonrpc":"2.0","id":"25","method":"org.rdk.FbPrivacy.1.setAllowPrimaryBrowseAdTargeting","params":{"value":false}}' http://127.0.0.1:9998/jsonrpc

# Set allowPrimaryContentAdTargeting
curl -d '{"jsonrpc":"2.0","id":"26","method":"org.rdk.FbPrivacy.1.setAllowPrimaryContentAdTargeting","params":{"value":true}}' http://127.0.0.1:9998/jsonrpc

# Set allowProductAnalytics
curl -d '{"jsonrpc":"2.0","id":"27","method":"org.rdk.FbPrivacy.1.setAllowProductAnalytics","params":{"value":false}}' http://127.0.0.1:9998/jsonrpc

# Set allowRemoteDiagnostics
curl -d '{"jsonrpc":"2.0","id":"28","method":"org.rdk.FbPrivacy.1.setAllowRemoteDiagnostics","params":{"value":true}}' http://127.0.0.1:9998/jsonrpc

# Set allowResumePoints
curl -d '{"jsonrpc":"2.0","id":"29","method":"org.rdk.FbPrivacy.1.setAllowResumePoints","params":{"value":true}}' http://127.0.0.1:9998/jsonrpc

# Set allowUnentitledPersonalization
curl -d '{"jsonrpc":"2.0","id":"30","method":"org.rdk.FbPrivacy.1.setAllowUnentitledPersonalization","params":{"value":false}}' http://127.0.0.1:9998/jsonrpc

# Set allowUnentitledResumePoints
curl -d '{"jsonrpc":"2.0","id":"31","method":"org.rdk.FbPrivacy.1.setAllowUnentitledResumePoints","params":{"value":true}}' http://127.0.0.1:9998/jsonrpc

# Set allowWatchHistory
curl -d '{"jsonrpc":"2.0","id":"32","method":"org.rdk.FbPrivacy.1.setAllowWatchHistory","params":{"value":true}}' http://127.0.0.1:9998/jsonrpc
```
