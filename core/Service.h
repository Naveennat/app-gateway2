#pragma once
/*
 * Compatibility shim for legacy include layout: <core/Service.h>
 *
 * L0 tests instantiate plugin classes using:
 *   WPEFramework::Core::Service<T>::Create<Interface>()
 *
 * In the Thunder/WPEFramework SDK, Service/Services helpers live under
 * WPEFramework/core. Forward to the vendored SDK header that provides this.
 */
#include "../dependencies/install/include/WPEFramework/core/Services.h"
