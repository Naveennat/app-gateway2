#pragma once

// Local shim header to satisfy AppGateway plugin includes.
// AppGateway/AppGateway.h includes "IAppGateway.h" (quoted include),
// but the actual interface lives at app-gateway/IAppGateway.h (and also
// app-gateway/interfaces/IAppGateway.h).
//
// Forward to the top-level interface header so builds can find it without
// changing upstream include statements.
#include "../IAppGateway.h"
