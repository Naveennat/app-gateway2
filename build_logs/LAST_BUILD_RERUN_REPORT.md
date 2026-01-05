# AppGateway plugin build rerun report

Rerun script:
- `./build_plugin_appgateway.sh`

Latest rerun (timestamp):
- `20260105_055745`

Result:
- Exit code: `1` (FAIL)

Log files:
- Wrapper log (full stdout/stderr): `app-gateway2/build_plugin_appgateway.log`
- Appended build log: `app-gateway2/build_logs/build_plugin_appgateway_20260105_055745.log`
- Configure log: `app-gateway2/tests/l0/appgateway/coverage/build_plugin_appgateway_configure.log`
- Build log: `app-gateway2/tests/l0/appgateway/coverage/build_plugin_appgateway_build.log`

Result marker:
- `app-gateway2/build_plugin_appgateway.result.txt`

Packaged artifacts:
- `app-gateway2/build_logs/appgateway_build_artifacts_20260105_055745.tgz`

Key markers:
- Resolved include dirs printed by script:
  - `/home/kavia/workspace/code-generation/app-gateway2/helpers/rdkservices-comcast/helpers`
  - `/home/kavia/workspace/code-generation/app-gateway2/helpers/entservices-infra/helpers`
  - `/home/kavia/workspace/code-generation/app-gateway2/helpers`
  - `/home/kavia/workspace/code-generation/app-gateway2/interfaces`
- However `APPGATEWAY_EXTRA_INCLUDE_DIRS=` was empty when passed to CMake.

Primary errors:
- `WsManager.h` not found
- `UtilsLogging.h` not found
