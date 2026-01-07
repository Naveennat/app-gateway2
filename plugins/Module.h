#pragma once
/*
 * Compatibility shim for legacy include layout: <plugins/Module.h>
 *
 * IMPORTANT:
 *  - L0 tests use Core::Service<T>::Create<>() and plugin headers use Thunder's
 *    interface-map macros (BEGIN_INTERFACE_MAP / INTERFACE_ENTRY / ...).
 *  - Our previous "no-op" interface-map macros broke compilation because plugin
 *    headers rely on these macros expanding to a real QueryInterface() override,
 *    and they also rely on the real PluginHost::IPlugin/IShell types.
 *
 * Approach:
 *  - Include WPEFramework core Services.h to get:
 *      - Core::Service<>::Create<>
 *      - BEGIN_INTERFACE_MAP / INTERFACE_ENTRY / INTERFACE_AGGREGATE / END_INTERFACE_MAP
 *      - SERVICE_REGISTRATION (harmless for unit tests)
 *  - Avoid including the full Thunder plugins/Module.h (which tends to drag in
 *    websocket/messaging stacks) and keep this header lightweight.
 */

#include "../interfaces/definitions.h"
#include "../UtilsLoggingAliases.h"

#include <core/Services.h>
#include <plugins/IPlugin.h>
#include <plugins/IDispatcher.h>
