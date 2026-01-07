#pragma once
/*
 * Compatibility shim for legacy include layout: <core/JSON.h>
 *
 * The vendored WPEFramework header lives under
 * dependencies/install/include/WPEFramework/core/JSON.h, but the isolated build
 * does not add dependencies/install/include to the include path.
 */
#include "../dependencies/install/include/WPEFramework/core/JSON.h"
