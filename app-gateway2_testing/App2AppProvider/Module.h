#pragma once
#ifndef MODULE_NAME
#define MODULE_NAME Plugin_App2AppProvider
#endif

// Pull in Thunder plugin framework core includes via the plugins umbrella header
#include <plugins/plugins.h>
#include <tracing/tracing.h>

// Ensure EXTERNAL is defined for symbol visibility, consistent with entservices-infra plugins
#undef EXTERNAL
#define EXTERNAL
