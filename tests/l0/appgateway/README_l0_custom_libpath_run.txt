AppGateway L0 Test Custom LibPath Instructions
---------------------------------------------

This test sequence ensures the L0 suite for AppGateway links and runs against the locally-installed Thunder/WPEFramework libraries:

1. The run_coverage.sh test script is now updated to always set:
   LD_LIBRARY_PATH = <repo>/dependencies/install/lib:<repo>/dependencies/install/lib/plugins:<repo>/dependencies/install/lib/wpeframework/plugins:$LD_LIBRARY_PATH

2. To run the suite with logging and automated error/summary extraction:
   $ cd app-gateway2/tests/l0/appgateway
   $ bash run_l0_with_custom_libpath_and_report.sh

   - This creates `l0_run_full.log` (full output) and `l0_run_summary.txt` (filtered summary).

3. Examine l0_run_summary.txt for:
   - Number of test PASS/FAIL events
   - Any loader, ABI, undefined symbol, or crash issues that are visible in test output

4. If no critical errors are present and most/all tests PASS, the dynamic library path setup is correct.

5. If there are persistent loader or ABI mismatches, double-check:
   - All required framework libs are present in the <repo>/dependencies/install/lib directory
   - That no system/global Thunder/WPEFramework libraries are being picked up at runtime

6. For further diagnostics, grep the log for common shared library or plugin-related errors.

Full logs are retained for further analysis.
