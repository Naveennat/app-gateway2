#pragma once
/*
 * Compatibility shim for legacy include layout: <plugins/IDispatcher.h>
 *
 * L0 tests and some plugin headers include this file via the legacy include path.
 * In this isolated repository build we use vendored Thunder/WPEFramework headers
 * under dependencies/install/include/WPEFramework.
 *
 * This shim forwards to the vendored SDK header without pulling in additional
 * repo-specific or heavy subsystems.
 */
#include "../dependencies/install/include/WPEFramework/plugins/IDispatcher.h"
