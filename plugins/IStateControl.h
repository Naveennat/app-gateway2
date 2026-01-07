#pragma once
/*
 * Compatibility shim for legacy include layout: <plugins/IStateControl.h>
 *
 * Some vendored WPEFramework interface headers include <plugins/IStateControl.h>.
 * Provide a repo-root forwarding header so the isolated build can resolve it.
 */
#include "../dependencies/install/include/WPEFramework/plugins/IStateControl.h"
