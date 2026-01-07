/*
 * L0 test-local wrapper for StringUtils.
 *
 * The test suite includes "StringUtils.h" with quotes (local include), expecting
 * StringUtils::rfindInsensitive and other helpers.
 *
 * Forward to the existing helper implementation used throughout this repo.
 */
#pragma once

#include "../../../../helpers/rdkservices-comcast/helpers/StringUtils.h"
