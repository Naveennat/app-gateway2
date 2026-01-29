#pragma once

// Upstream AppGateway sources sometimes include "IAppGateway.h" as a local header.
// In this repository, the canonical interface header is located under
// `app-gateway2/interfaces/IAppGateway.h`.
// This shim keeps the hard-synced upstream sources unmodified while allowing
// isolated builds and L1 tests to compile.
#include <interfaces/IAppGateway.h>
