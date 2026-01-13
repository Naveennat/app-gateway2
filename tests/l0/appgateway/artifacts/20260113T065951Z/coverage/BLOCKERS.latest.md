# AppGateway L0 coverage run (latest)

- date: 2026-01-05T07:11:38+00:00
- exit_code: 134

## Plugin presence
-rwxrwxrwx 1 kavia kavia 7223856 Jan  5 06:42 dependencies/install/lib/plugins/libWPEFrameworkAppGateway.so
ls: cannot access 'dependencies/install/lib/wpeframework/plugins/libWPEFrameworkAppGateway.so': No such file or directory

## Coverage outputs
total 440
drwxrwxrwx 2 kavia kavia   4096 Jan  5 07:05 .
drwxrwxrwx 4 kavia kavia   4096 Jan  5 07:08 ..
-rwxrwxrwx 1 kavia kavia   4161 Jan  5 05:37 appgateway_build_20260105-053731.log
-rwxrwxrwx 1 kavia kavia   4161 Jan  5 05:37 appgateway_build_20260105-053748.log
-rwxrwxrwx 1 kavia kavia    970 Jan  5 07:03 ARTIFACTS.md
-rwxrwxrwx 1 kavia kavia    361 Jan  5 07:11 BLOCKERS.latest.md
-rwxrwxrwx 1 kavia kavia  51441 Jan  5 05:01 build_plugin_appgateway_build2.log
-rwxrwxrwx 1 kavia kavia   1177 Jan  5 06:42 build_plugin_appgateway_build.log
-rwxrwxrwx 1 kavia kavia   5371 Jan  5 06:41 build_plugin_appgateway_configure.log
-rwxrwxrwx 1 kavia kavia    581 Jan  5 06:37 build_plugin_appgateway_errors.latest.txt
-rwxrwxrwx 1 kavia kavia   1739 Jan  5 05:28 build_plugin_appgateway_run.log
-rwxrwxrwx 1 kavia kavia    344 Jan  5 06:37 BUILD_RERUN_MARKER.txt
-rwxrwxrwx 1 kavia kavia    664 Jan  5 05:27 BUILD_RESULT.marker.txt
-rwxrwxrwx 1 kavia kavia  47405 Jan  5 07:08 console.latest.txt
-rwxrwxrwx 1 kavia kavia  11047 Jan  5 06:58 console.log
-rwxrwxrwx 1 kavia kavia  29521 Jan  5 07:00 console_output_after_include_fix.txt
-rwxrwxrwx 1 kavia kavia  43294 Jan  5 07:01 console_output.txt
-rwxrwxrwx 1 kavia kavia      1 Jan  5 06:53 .gitkeep
-rwxrwxrwx 1 kavia kavia   4858 Jan  5 07:02 manual_test_run.txt
-rwxrwxrwx 1 kavia kavia  12037 Jan  5 05:39 rebuild_20260105.log
-rwxrwxrwx 1 kavia kavia  12037 Jan  5 05:39 rebuild_after_fixes_20260105.log
-rwxrwxrwx 1 kavia kavia 108075 Jan  5 07:11 RESULT.latest.txt
-rwxrwxrwx 1 kavia kavia  48287 Jan  5 06:50 run_coverage.latest.log

No coverage.info generated (run failed before lcov capture or crashed).

## Top errors (grep)
335:[1;32m[Services.cpp:90](Instantiate)<PID:65127><TID:65127><1>: Missing implementation classname AppGatewayImplementation in library 
337:[0m[65127] ERROR [AppGateway.cpp:85] Initialize: Failed to initialise AppGatewayResolver plugin!
344:FAIL: Initialize() returns empty string on success expected='' actual='Could not retrieve the AppGateway interface.'
353:[1;32m[Services.cpp:90](Instantiate)<PID:65127><TID:65127><1>: Missing implementation classname AppGatewayImplementation in library 
355:[0m[65127] ERROR [AppGateway.cpp:85] Initialize: Failed to initialise AppGatewayResolver plugin!
361:NOTE: Initialize() returned non-empty error string; recording but continuing: Could not retrieve the AppGateway interface.
367:[1;32m[Services.cpp:90](Instantiate)<PID:65127><TID:65127><1>: Missing implementation classname AppGatewayImplementation in library 
369:[0m[65127] ERROR [AppGateway.cpp:85] Initialize: Failed to initialise AppGatewayResolver plugin!
375:NOTE: Second Initialize() (fresh instance) returned non-empty error string; recording but continuing: Could not retrieve the AppGateway interface.
383:[1;32m[Services.cpp:90](Instantiate)<PID:65127><TID:65127><1>: Missing implementation classname AppGatewayImplementation in library 
385:[0m[65127] ERROR [AppGateway.cpp:85] Initialize: Failed to initialise AppGatewayResolver plugin!
391:FAIL: Initialize() succeeds before first Deinitialize() expected='' actual='Could not retrieve the AppGateway interface.'
396:[1;32m[Services.cpp:90](Instantiate)<PID:65127><TID:65127><1>: Missing implementation classname AppGatewayImplementation in library 
398:[0m[65127] ERROR [AppGateway.cpp:85] Initialize: Failed to initialise AppGatewayResolver plugin!
404:FAIL: Re-Initialize() succeeds before second Deinitialize() expected='' actual='Could not retrieve the AppGateway interface.'
412:[1;32m[Services.cpp:90](Instantiate)<PID:65127><TID:65127><1>: Missing implementation classname AppGatewayImplementation in library 
414:[0m[65127] ERROR [AppGateway.cpp:85] Initialize: Failed to initialise AppGatewayResolver plugin!
420:FAIL: Initialize() returns empty string on success expected='' actual='Could not retrieve the AppGateway interface.'
430:[1;32m[Services.cpp:90](Instantiate)<PID:65127><TID:65127><1>: Missing implementation classname AppGatewayImplementation in library 
432:[0m[65127] ERROR [AppGateway.cpp:85] Initialize: Failed to initialise AppGatewayResolver plugin!
448:[1;32m[Services.cpp:90](Instantiate)<PID:65127><TID:65127><1>: Missing implementation classname AppGatewayImplementation in library 
450:[0m[65127] ERROR [AppGateway.cpp:85] Initialize: Failed to initialise AppGatewayResolver plugin!
469:[1;32m[Services.cpp:90](Instantiate)<PID:65127><TID:65127><1>: Missing implementation classname AppGatewayImplementation in library 
471:[0m[65127] ERROR [AppGateway.cpp:85] Initialize: Failed to initialise AppGatewayResolver plugin!
487:[1;32m[Services.cpp:90](Instantiate)<PID:65127><TID:65127><1>: Missing implementation classname AppGatewayImplementation in library 
489:[0m[65127] ERROR [AppGateway.cpp:85] Initialize: Failed to initialise AppGatewayResolver plugin!
505:[1;32m[Services.cpp:90](Instantiate)<PID:65127><TID:65127><1>: Missing implementation classname AppGatewayImplementation in library 
507:[0m[65127] ERROR [AppGateway.cpp:85] Initialize: Failed to initialise AppGatewayResolver plugin!
513:FAIL: Initialize() happy path returns empty string expected='' actual='Could not retrieve the AppGateway interface.'
521:[1;32m[Services.cpp:90](Instantiate)<PID:65127><TID:65127><1>: Missing implementation classname AppGatewayImplementation in library 
523:[0m[65127] ERROR [AppGateway.cpp:85] Initialize: Failed to initialise AppGatewayResolver plugin!
529:FAIL: Initialize() succeeds for JSON-RPC resolve test expected='' actual='Could not retrieve the AppGateway interface.'
539:[1;32m[Services.cpp:90](Instantiate)<PID:65127><TID:65127><1>: Missing implementation classname AppGatewayImplementation in library 
541:[0m[65127] ERROR [AppGateway.cpp:85] Initialize: Failed to initialise AppGatewayResolver plugin!
547:FAIL: Initialize() succeeds for direct resolver test expected='' actual='Could not retrieve the AppGateway interface.'
556:[1;32m[Services.cpp:90](Instantiate)<PID:65127><TID:65127><1>: Missing implementation classname AppGatewayImplementation in library 
558:[0m[65127] ERROR [AppGateway.cpp:85] Initialize: Failed to initialise AppGatewayResolver plugin!
564:FAIL: Initialize() succeeds for NotPermitted test expected='' actual='Could not retrieve the AppGateway interface.'
573:[1;32m[Services.cpp:90](Instantiate)<PID:65127><TID:65127><1>: Missing implementation classname AppGatewayImplementation in library 
575:[0m[65127] ERROR [AppGateway.cpp:85] Initialize: Failed to initialise AppGatewayResolver plugin!
581:FAIL: Initialize() succeeds for NotSupported test expected='' actual='Could not retrieve the AppGateway interface.'
590:[1;32m[Services.cpp:90](Instantiate)<PID:65127><TID:65127><1>: Missing implementation classname AppGatewayImplementation in library 
592:[0m[65127] ERROR [AppGateway.cpp:85] Initialize: Failed to initialise AppGatewayResolver plugin!
598:FAIL: Initialize() succeeds for NotAvailable test expected='' actual='Could not retrieve the AppGateway interface.'
607:[1;32m[Services.cpp:90](Instantiate)<PID:65127><TID:65127><1>: Missing implementation classname AppGatewayImplementation in library 
609:[0m[65127] ERROR [AppGateway.cpp:85] Initialize: Failed to initialise AppGatewayResolver plugin!
615:FAIL: Initialize() succeeds for malformed input test expected='' actual='Could not retrieve the AppGateway interface.'
624:[1;32m[Services.cpp:90](Instantiate)<PID:65127><TID:65127><1>: Missing implementation classname AppGatewayImplementation in library 
626:[0m[65127] ERROR [AppGateway.cpp:85] Initialize: Failed to initialise AppGatewayResolver plugin!
641:[1;32m[Services.cpp:90](Instantiate)<PID:65127><TID:65127><1>: Missing implementation classname AppGatewayImplementation in library 
643:[0m[65127] ERROR [AppGateway.cpp:85] Initialize: Failed to initialise AppGatewayResolver plugin!
813:double free or corruption (out)
