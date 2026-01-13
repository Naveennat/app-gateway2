/*
 * Local forwarding header for AppGateway plugin builds in this repository.
 *
 * Upstream builds typically inject helper include paths so that
 *   #include "WsManager.h"
 * resolves from a shared helpers folder.
 *
 * Our isolated CI/coverage builds can miss those include directories,
 * causing compilation failures.
 *
 * This header keeps the plugin sources unchanged while ensuring the
 * correct helper header is available.
 */

#pragma once

#include "../../helpers/rdkservices-comcast/helpers/WsManager.h"
