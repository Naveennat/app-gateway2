#pragma once

/*
 * Minimal ErrorUtils for L0/unit-test builds.
 *
 * The L0 tests only require that these functions exist and produce a
 * deterministic string payload. This header is included via:
 *   AppGateway/AppGateway.h -> ../helpers/ErrorUtils.h
 *   AppGateway/AppGatewayImplementation.cpp -> ../helpers/ErrorUtils.h
 */

#include <string>

namespace AppGwErrorUtils {

struct ErrorUtils {
    static void CustomInitialize(const std::string& message, std::string& out)
    {
        out = std::string("{\"error\":\"Initialize\",\"message\":\"") + message + "\"}";
    }

    static void CustomInternal(const std::string& message, std::string& out)
    {
        out = std::string("{\"error\":\"Internal\",\"message\":\"") + message + "\"}";
    }

    static void CustomBadRequest(const std::string& message, std::string& out)
    {
        out = std::string("{\"error\":\"BadRequest\",\"message\":\"") + message + "\"}";
    }

    static void NotSupported(std::string& out)
    {
        out = "{\"error\":\"NotSupported\"}";
    }

    static void NotAvailable(std::string& out)
    {
        out = "{\"error\":\"NotAvailable\"}";
    }
};

inline void NotPermitted(std::string& out)
{
    out = "{\"error\":\"NotPermitted\"}";
}

} // namespace AppGwErrorUtils
