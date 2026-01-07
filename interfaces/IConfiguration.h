#pragma once
/*
 * Compatibility shim for legacy include layout: <interfaces/IConfiguration.h>
 *
 * AppGateway plugin sources include this header from the Thunder SDK. In this
 * repository we depend on the vendored WPEFramework headers under
 * dependencies/install/include/WPEFramework.
 */
#include "../dependencies/install/include/WPEFramework/interfaces/IConfiguration.h"
