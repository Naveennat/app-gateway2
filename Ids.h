#pragma once
/*
 * Compatibility shim for legacy include layout: <Ids.h>
 *
 * Some plugin sources include <Ids.h> expecting it to be available on the
 * global include path. In this repository, the header lives under
 * app-gateway2/interfaces/Ids.h. Provide a repo-root forwarding header so the
 * isolated plugin build can find it without extra include flags.
 */
#include "interfaces/Ids.h"
