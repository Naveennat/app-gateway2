#pragma once
/*
 * Compatibility shim for legacy include layout: <com/ICOM.h>
 *
 * The plugin build includes the repository root (app-gateway2/) in its include
 * paths. Provide this shim so vendored WPEFramework headers can include
 * <com/ICOM.h> without requiring additional include flags.
 */
#include "dependencies/install/include/WPEFramework/com/ICOM.h"
