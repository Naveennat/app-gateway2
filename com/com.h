#pragma once
/*
 * Minimal compatibility shim for legacy include layout: <com/com.h>
 *
 * The full WPEFramework <com/com.h> pulls in a large dependency chain (including
 * messaging). In this repository's isolated build we only require the base COM
 * interface definitions.
 */
#include "ICOM.h"
