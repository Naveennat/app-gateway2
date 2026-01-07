#pragma once
/*
 * Compatibility shim for legacy include layout: <plugins/IShell.h>
 *
 * The plugin build includes the repository root (app-gateway2/) in its include
 * paths. Provide this shim so source files can include <plugins/IShell.h>
 * without requiring additional include flags.
 */
#include "../dependencies/install/include/WPEFramework/plugins/IShell.h"
