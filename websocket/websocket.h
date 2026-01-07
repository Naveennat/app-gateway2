#pragma once
/*
 * Compatibility shim for legacy include layout: <websocket/websocket.h>
 *
 * The plugin build includes the repository root (app-gateway2/) in its include
 * paths. Provide this shim so vendored WPEFramework headers that include
 * <websocket/websocket.h> can be found without requiring extra include flags.
 */
#include "dependencies/install/include/websocket/websocket.h"
