#pragma once
/*
 * Logging shim for AppGateway plugin sources.
 *
 * Upstream builds provide UtilsLogging.h via shared helper infrastructure.
 * In this isolated repository we map it to our no-op logging macro aliases.
 *
 * IMPORTANT:
 *  - Do not redefine Thunder SDK macros (ASSERT, TRACE_Lx, interface map macros, etc.).
 *  - Keep this header minimal and safe to include from any TU.
 */
#include "../../UtilsLoggingAliases.h"
