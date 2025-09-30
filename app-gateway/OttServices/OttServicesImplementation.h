#pragma once

// Internal implementation for the OttServices plugin.
// Contains business logic and state, separated from the Thunder plugin facade.

#include <core/core.h>
#include <plugins/IShell.h>

namespace WPEFramework {
namespace Plugin {

    class OttServicesImplementation {
    public:
        OttServicesImplementation(const OttServicesImplementation&) = delete;
        OttServicesImplementation& operator=(const OttServicesImplementation&) = delete;

        // PUBLIC_INTERFACE
        OttServicesImplementation();
        // PUBLIC_INTERFACE
        ~OttServicesImplementation();

        // PUBLIC_INTERFACE
        string Initialize(PluginHost::IShell* service);
        // PUBLIC_INTERFACE
        void Deinitialize(PluginHost::IShell* service);

        // PUBLIC_INTERFACE
        uint32_t Ping(const string& message, string& reply);

    private:
        PluginHost::IShell* _service;
        string _state;
    };

} // namespace Plugin
} // namespace WPEFramework
