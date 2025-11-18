#pragma once

/*
 * Wrapper header to locate the actual OttPermissionCache implementation.
 * Keeps existing includes working:
 *   #include "OttServices/OttPermissionCache.h"
 *
 * The implementation currently resides under ProtoImplementations/ThorPermissions.
 */

#include "ProtoImplementations/ThorPermissions/OttPermissionCache.h"
