#pragma once
/*
 * Compatibility shim for legacy include layout: <plugins/ISubSystem.h>
 *
 * Some vendored WPEFramework interface headers include <plugins/ISubSystem.h>.
 * Provide a repo-root forwarding header so the isolated build can resolve it.
 */
#include "../dependencies/install/include/WPEFramework/plugins/ISubSystem.h"
