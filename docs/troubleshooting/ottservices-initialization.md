# OttServices plugin initialization troubleshooting

If OttServices logs show:
- "OttServices::Initialize: Failed to initialise OttServices plugin" and/or
- "Activation of plugin [OttServices]:[org.rdk.OttServices], failed. Error [Could not retrieve the OttServices interface.]"

then the adapter could not bind to the out-of-process IOttServices.

The plugin now starts in degraded mode (no bound IOttServices) and logs actionable steps. To recover:

1) Ensure the implementation plugin is configured
   - Create a plugin config (path depends on your WPEFramework setup, usually /etc/Thunder/plugins/):
     {
       "callsign": "OttServicesImplementation",
       "locator": "libOttServicesImplementation.so",
       "classname": "OttServicesImplementation",
       "autostart": true
     }
   - Adjust callsign as desired. className must match OttServicesImplementation.

2) Ensure Proxy/Stub for IOttServices is present
   - The Exchange::IOttServices COM-RPC proxy/stub must be compiled and placed under the configured proxystub path.
   - Confirm the interface ID matches on both sides: ID_OTT_SERVICES = 0x0000F812.

3) Match class name or update Root() usage
   - The adapter tries Root(..., "OttServicesImplementation"). If your implementation uses a different class name, update the adapter or config accordingly.

4) Optionally activate via Controller
   - You can activate the implementation plugin using the Controller.LifeTime.Activate call.

Notes:
- While degraded, OttServices remains activated to avoid startup failures but will not expose functional IOttServices-backed methods.
- After fixing the above, deactivate and reactivate the OttServices plugin or restart WPEFramework to bind to the implementation.
