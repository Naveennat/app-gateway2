#pragma once
/*
 * Forwarding shim for legacy include layout: <plugins/IPlugin.h>
 *
 * AppGateway inherits from PluginHost::IPlugin and expects associated types
 * like PluginHost::IShell and interface-map macros to be available.
 */
#include "../dependencies/install/include/WPEFramework/plugins/IPlugin.h"
