#pragma once
/*
 * Compatibility shim for legacy include layout: <com/IIteratorType.h>
 *
 * Some repo-local interface headers use:
 *   RPC::IIteratorType<string, RPC::ID_STRINGITERATOR>
 * and assume the iterator template and the ID_* constants are both visible.
 *
 * The vendored SDK provides:
 *   - WPEFramework/com/Ids.h         (ID_STRINGITERATOR etc.)
 *   - WPEFramework/com/IIteratorType.h (IIteratorType template)
 *
 * Include order matters for older/trimmed builds, so include Ids first.
 */

#include "../dependencies/install/include/WPEFramework/com/Ids.h"
#include "../dependencies/install/include/WPEFramework/com/IIteratorType.h"
