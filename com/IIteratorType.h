#pragma once
/*
 * Compatibility shim for legacy include layout: <com/IIteratorType.h>
 *
 * The AppGateway interfaces use RPC::IIteratorType and L0 tests include the
 * iterator template via the legacy include path. Forward to the vendored SDK.
 */
#include "../dependencies/install/include/WPEFramework/com/IIteratorType.h"
