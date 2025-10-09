#pragma once

/*
 * Minimal stub for the Exchange::IOttServices interface to satisfy build-time includes.
 * This header defines the interface used by OttServices and OttServicesImplementation.
 * Note: The ID value is a placeholder for compilation purposes only.
 */

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IOttServices : virtual public Core::IUnknown {
        // Placeholder interface ID; in a full Thunder environment, this would be defined in Ids.h
        enum { ID = 0xFA100001 };

        virtual ~IOttServices() = default;

        // Keep this interface intentionally minimal; methods are defined in the remote implementation.
    };

} // namespace Exchange
} // namespace WPEFramework
