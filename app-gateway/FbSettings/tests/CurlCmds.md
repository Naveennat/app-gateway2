# FbSettings JSON-RPC cURL Commands

All examples use the default Thunder JSON-RPC endpoint at:
  http://127.0.0.1:9998/jsonrpc

Adjust host and/or port as needed for your environment.

## Controller: Activate / Deactivate org.rdk.FbSettings

These operations help you start/stop the FbSettings plugin before running the method tests.

- Activate FbSettings
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":1001,"method":"Controller.1.activate","params":{"callsign":"org.rdk.FbSettings"}}' \
  http://127.0.0.1:9998/jsonrpc
```

- Deactivate FbSettings
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":1002,"method":"Controller.1.deactivate","params":{"callsign":"org.rdk.FbSettings"}}' \
  http://127.0.0.1:9998/jsonrpc
```

---

## org.rdk.FbSettings.1: org.rdk.System callsign-derived aliases

Below are example requests for each of the 13 methods exposed by org.rdk.FbSettings.1. 
- For getters/response-only methods, no params are included.
- For methods that require input, an example params object is provided.
- For subscribe methods, the listen parameter is demonstrated.

1) getDeviceMake
- Tests retrieving the device make (maps to org.rdk.System.getDeviceInfo)
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":1,"method":"org.rdk.FbSettings.1.getDeviceMake"}' \
  http://127.0.0.1:9998/jsonrpc
```

2) getDeviceName
- Tests retrieving the device friendly name (maps to org.rdk.System.getFriendlyName)
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":2,"method":"org.rdk.FbSettings.1.getDeviceName"}' \
  http://127.0.0.1:9998/jsonrpc
```

3) setDeviceName
- Tests setting the device friendly name (maps to org.rdk.System.setFriendlyName)
- Example provides a sample name
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":3,"method":"org.rdk.FbSettings.1.setDeviceName","params":{"name":"Living Room"}}' \
  http://127.0.0.1:9998/jsonrpc
```

4) getDeviceSku
- Tests retrieving the device SKU (maps to org.rdk.System.getSystemVersions)
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":4,"method":"org.rdk.FbSettings.1.getDeviceSku"}' \
  http://127.0.0.1:9998/jsonrpc
```

5) getCountryCode
- Tests retrieving the country code (maps to org.rdk.System.getTerritory -> Firebolt country code)
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":5,"method":"org.rdk.FbSettings.1.getCountryCode"}' \
  http://127.0.0.1:9998/jsonrpc
```

6) setCountryCode
- Tests setting the country code (maps to org.rdk.System.setTerritory)
- Example provides a sample country code (US)
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":6,"method":"org.rdk.FbSettings.1.setCountryCode","params":{"countryCode":"US"}}' \
  http://127.0.0.1:9998/jsonrpc
```

7) subscribeOnCountryCodeChanged
- Tests subscribing to country code change events
- Example subscribes (listen: true). Use false to unsubscribe.
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":7,"method":"org.rdk.FbSettings.1.subscribeOnCountryCodeChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

8) getTimeZone
- Tests retrieving the current timezone (maps to org.rdk.System.getTimeZoneDST)
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":8,"method":"org.rdk.FbSettings.1.getTimeZone"}' \
  http://127.0.0.1:9998/jsonrpc
```

9) setTimeZone
- Tests setting the current timezone (maps to org.rdk.System.setTimeZoneDST)
- Example provides an illustrative timezone
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":9,"method":"org.rdk.FbSettings.1.setTimeZone","params":{"timeZone":"America/New_York"}}' \
  http://127.0.0.1:9998/jsonrpc
```

10) subscribeOnTimeZoneChanged
- Tests subscribing to timezone change events
- Example subscribes (listen: true). Use false to unsubscribe.
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":10,"method":"org.rdk.FbSettings.1.subscribeOnTimeZoneChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

11) getSecondScreenFriendlyName
- Tests retrieving the second screen friendly name (alias of getDeviceName)
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":11,"method":"org.rdk.FbSettings.1.getSecondScreenFriendlyName"}' \
  http://127.0.0.1:9998/jsonrpc
```

12) subscribeOnFriendlyNameChanged
- Tests subscribing to friendly name change events
- Example subscribes (listen: true). Use false to unsubscribe.
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":12,"method":"org.rdk.FbSettings.1.subscribeOnFriendlyNameChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

13) subscribeOnDeviceNameChanged
- Tests subscribing to device name change events (alias of friendly name changed)
- Example subscribes (listen: true). Use false to unsubscribe.
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":13,"method":"org.rdk.FbSettings.1.subscribeOnDeviceNameChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

---
## org.rdk.FbSettings.1: org.rdk.UserSettings callsign-derived aliases (35)

1) getLanguage
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":201,"method":"org.rdk.FbSettings.1.getLanguage"}' \
  http://127.0.0.1:9998/jsonrpc
```

2) subscribeOnLanguageChanged
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":202,"method":"org.rdk.FbSettings.1.subscribeOnLanguageChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

3) getLocale
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":203,"method":"org.rdk.FbSettings.1.getLocale"}' \
  http://127.0.0.1:9998/jsonrpc
```

4) setLocale
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":204,"method":"org.rdk.FbSettings.1.setLocale","params":{"locale":"en-US"}}' \
  http://127.0.0.1:9998/jsonrpc
```

5) subscribeOnLocaleChanged
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":205,"method":"org.rdk.FbSettings.1.subscribeOnLocaleChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

6) getPreferredAudioLanguages
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":206,"method":"org.rdk.FbSettings.1.getPreferredAudioLanguages"}' \
  http://127.0.0.1:9998/jsonrpc
```

7) setPreferredAudioLanguages
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":207,"method":"org.rdk.FbSettings.1.setPreferredAudioLanguages","params":{"languages":"eng,spa"}}' \
  http://127.0.0.1:9998/jsonrpc
```

8) subscribeOnPreferredAudioLanguagesChanged
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":208,"method":"org.rdk.FbSettings.1.subscribeOnPreferredAudioLanguagesChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

9) setVoiceGuidanceEnabled
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":209,"method":"org.rdk.FbSettings.1.setVoiceGuidanceEnabled","params":{"enabled":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

10) setVoiceGuidanceSpeed
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":210,"method":"org.rdk.FbSettings.1.setVoiceGuidanceSpeed","params":{"speed":50}}' \
  http://127.0.0.1:9998/jsonrpc
```

11) subscribeOnAudioDescriptionSettingsChanged
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":211,"method":"org.rdk.FbSettings.1.subscribeOnAudioDescriptionSettingsChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

12) getAudioDescriptionSettings
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":212,"method":"org.rdk.FbSettings.1.getAudioDescriptionSettings"}' \
  http://127.0.0.1:9998/jsonrpc
```

13) getAudioDescriptionsEnabled
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":213,"method":"org.rdk.FbSettings.1.getAudioDescriptionsEnabled"}' \
  http://127.0.0.1:9998/jsonrpc
```

14) setAudioDescriptionsEnabled
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":214,"method":"org.rdk.FbSettings.1.setAudioDescriptionsEnabled","params":{"enabled":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

15) subscribeOnAudioDescriptionsEnabledChanged
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":215,"method":"org.rdk.FbSettings.1.subscribeOnAudioDescriptionsEnabledChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

16) getHighContrastUI
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":216,"method":"org.rdk.FbSettings.1.getHighContrastUI"}' \
  http://127.0.0.1:9998/jsonrpc
```

17) subscribeOnHighContrastUIChanged
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":217,"method":"org.rdk.FbSettings.1.subscribeOnHighContrastUIChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

18) getClosedCaptionsEnabled
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":218,"method":"org.rdk.FbSettings.1.getClosedCaptionsEnabled"}' \
  http://127.0.0.1:9998/jsonrpc
```

19) setClosedCaptionsEnabled
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":219,"method":"org.rdk.FbSettings.1.setClosedCaptionsEnabled","params":{"enabled":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

20) subscribeOnClosedCaptionsEnabledChanged
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":220,"method":"org.rdk.FbSettings.1.subscribeOnClosedCaptionsEnabledChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

21) getClosedCaptionsPreferredLanguages
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":221,"method":"org.rdk.FbSettings.1.getClosedCaptionsPreferredLanguages"}' \
  http://127.0.0.1:9998/jsonrpc
```

22) setClosedCaptionsPreferredLanguages
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":222,"method":"org.rdk.FbSettings.1.setClosedCaptionsPreferredLanguages","params":{"languages":"eng,spa"}}' \
  http://127.0.0.1:9998/jsonrpc
```

23) subscribeOnClosedaptionsPreferredLanguagesChanged (intentional typo)
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":223,"method":"org.rdk.FbSettings.1.subscribeOnClosedaptionsPreferredLanguagesChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

24) subscribeOnClosedCaptionsSettingsChanged
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":224,"method":"org.rdk.FbSettings.1.subscribeOnClosedCaptionsSettingsChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

25) getVoiceGuidanceNavigationHints
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":225,"method":"org.rdk.FbSettings.1.getVoiceGuidanceNavigationHints"}' \
  http://127.0.0.1:9998/jsonrpc
```

26) setVoiceGuidanceNavigationHints
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":226,"method":"org.rdk.FbSettings.1.setVoiceGuidanceNavigationHints","params":{"enabled":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

27) subscribeOnVoiceGuidanceNavigationHintsChanged
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":227,"method":"org.rdk.FbSettings.1.subscribeOnVoiceGuidanceNavigationHintsChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

28) getVoiceGuidanceRate
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":228,"method":"org.rdk.FbSettings.1.getVoiceGuidanceRate"}' \
  http://127.0.0.1:9998/jsonrpc
```

29) setVoiceGuidanceRate
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":229,"method":"org.rdk.FbSettings.1.setVoiceGuidanceRate","params":{"rate":50.0}}' \
  http://127.0.0.1:9998/jsonrpc
```

30) subscribeOnVoiceGuidanceRateChanged
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":230,"method":"org.rdk.FbSettings.1.subscribeOnVoiceGuidanceRateChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

31) getVoiceGuidanceEnabled
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":231,"method":"org.rdk.FbSettings.1.getVoiceGuidanceEnabled"}' \
  http://127.0.0.1:9998/jsonrpc
```

32) subscribeOnVoiceGuidanceEnabledChanged
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":232,"method":"org.rdk.FbSettings.1.subscribeOnVoiceGuidanceEnabledChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

33) getVoiceGuidanceSpeed
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":233,"method":"org.rdk.FbSettings.1.getVoiceGuidanceSpeed"}' \
  http://127.0.0.1:9998/jsonrpc
```

34) subscribeOnVoiceGuidanceSpeedChanged
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":234,"method":"org.rdk.FbSettings.1.subscribeOnVoiceGuidanceSpeedChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```

35) subscribeOnVoiceGuidanceSettingsChanged
```bash
curl -X POST -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":235,"method":"org.rdk.FbSettings.1.subscribeOnVoiceGuidanceSettingsChanged","params":{"listen":true}}' \
  http://127.0.0.1:9998/jsonrpc
```
