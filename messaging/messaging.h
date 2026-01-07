#pragma once
/*
 * Compatibility shim for legacy include layout: <messaging/messaging.h>
 *
 * In this repo, WPEFramework headers are vendored under:
 *   dependencies/install/include/WPEFramework/...
 *
 * Some code (or transitive includes) may still include <messaging/messaging.h>.
 * Provide this shim so those includes resolve correctly without requiring
 * additional include flags or a full Thunder SDK layout.
 */
#include "dependencies/install/include/WPEFramework/messaging/messaging.h"
