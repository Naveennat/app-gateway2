# CMake generated Testfile for 
# Source directory: /home/kavia/workspace/code-generation/app-gateway2/app-gateway/tests
# Build directory: /home/kavia/workspace/code-generation/app-gateway2/build-local/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(AppGateway.Basic "/home/kavia/workspace/code-generation/app-gateway2/build-local/tests/test_appgateway")
set_tests_properties(AppGateway.Basic PROPERTIES  _BACKTRACE_TRIPLES "/home/kavia/workspace/code-generation/app-gateway2/app-gateway/tests/CMakeLists.txt;73;add_test;/home/kavia/workspace/code-generation/app-gateway2/app-gateway/tests/CMakeLists.txt;0;")
add_test(FbCommon.Resolutions "/home/kavia/workspace/code-generation/app-gateway2/build-local/tests/test_fbcommon")
set_tests_properties(FbCommon.Resolutions PROPERTIES  _BACKTRACE_TRIPLES "/home/kavia/workspace/code-generation/app-gateway2/app-gateway/tests/CMakeLists.txt;105;add_test;/home/kavia/workspace/code-generation/app-gateway2/app-gateway/tests/CMakeLists.txt;0;")
add_test(OttPermissionCache.Idempotent "/home/kavia/workspace/code-generation/app-gateway2/build-local/tests/test_ottpermissioncache")
set_tests_properties(OttPermissionCache.Idempotent PROPERTIES  _BACKTRACE_TRIPLES "/home/kavia/workspace/code-generation/app-gateway2/app-gateway/tests/CMakeLists.txt;127;add_test;/home/kavia/workspace/code-generation/app-gateway2/app-gateway/tests/CMakeLists.txt;0;")
