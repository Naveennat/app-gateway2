#pragma once

/*
 * Minimal stub for WPEFramework "interfaces/definitions.h".
 *
 * In a full Thunder/WPEFramework SDK this header is provided by ThunderInterfaces
 * and contains interface-related helper macros and attributes.
 *
 * This repository builds the AppGateway plugin and runs L0 tests in isolation.
 * Only a small subset of that header is required for compilation of helper
 * WebSocket components that include <interfaces/definitions.h>.
 */

// Keep these macros minimal and compatible with typical upstream expectations.
#ifndef EXTERNAL
#define EXTERNAL
#endif

#ifndef EXTERNAL_REFERENCE
#define EXTERNAL_REFERENCE
#endif

#ifndef DEPRECATED
#if defined(__GNUC__) || defined(__clang__)
#define DEPRECATED __attribute__((deprecated))
#else
#define DEPRECATED
#endif
#endif

#ifndef NORETURN
#if defined(__GNUC__) || defined(__clang__)
#define NORETURN __attribute__((noreturn))
#else
#define NORETURN
#endif
#endif

#ifndef ALWAYS_INLINE
#if defined(__GNUC__) || defined(__clang__)
#define ALWAYS_INLINE inline __attribute__((always_inline))
#else
#define ALWAYS_INLINE inline
#endif
#endif
