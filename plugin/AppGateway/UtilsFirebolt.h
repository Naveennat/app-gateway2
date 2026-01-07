#pragma once

/*
 * Local wrapper header to ensure AppGateway plugin uses the repo's "safe"
 * UtilsFirebolt implementation (which does NOT define colliding ERROR_* macros).
 *
 * Without this wrapper, the build can pick up:
 *   dependencies/entservices-infra/helpers/UtilsFirebolt.h
 * which defines macros like ERROR_NOT_SUPPORTED, breaking references like:
 *   Core::ERROR_NOT_SUPPORTED
 */

// Pull in the repo-maintained, collision-free implementation.
// The build already adds <repo>/helpers to the include path.
#include <rdkservices-comcast/helpers/UtilsFirebolt.h>
