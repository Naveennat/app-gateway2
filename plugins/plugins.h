#pragma once
/*
 * Minimal plugin umbrella header used by this repository's AppGateway plugin.
 *
 * Upstream Thunder provides <plugins/plugins.h> which includes a wide set of
 * optional headers (e.g. VirtualInput) that are not required for AppGateway and
 * may not compile in this repo's isolated harness/shim environment.
 *
 * Keep this umbrella narrow: only include what AppGateway sources need.
 */
#include "Module.h"
#include "JSONRPC.h"
