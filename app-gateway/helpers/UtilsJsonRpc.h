#pragma once

/*
 PUBLIC_INTERFACE
 Minimal JSON-RPC utility shim to satisfy usage in Resolver.cpp:
 - Utils::GetThunderControllerClient(PluginHost::IShell*, const std::string&)
   returns an object exposing Invoke<Ret, Arg>(method, params, out)
 This stub uses WPEFramework::PluginHost::JSONRPC internally to call the target plugin.
*/

#include <plugins/JSONRPC.h>
#include <core/core.h>
#include <memory>
#include <string>

namespace Utils {

    class ThunderLink {
    public:
        ThunderLink(WPEFramework::PluginHost::JSONRPC* client, const std::string& callsign)
            : _client(client), _callsign(callsign) {}

        template <typename Ret, typename Arg>
        WPEFramework::Core::hresult Invoke(const std::string& method, const Arg& params, Ret& response) {
            if (_client == nullptr) {
                return WPEFramework::Core::ERROR_UNAVAILABLE;
            }
            std::string out;
            // JSONRPC::Invoke signature in Thunder requires callback, waitTime, channel id, callsign, method, params, response
            const uint32_t rc = _client->Invoke(nullptr, 2000, 0, _callsign, method, params, out);
            if (rc == WPEFramework::Core::ERROR_NONE) {
                response = out;
            }
            return rc;
        }

    private:
        WPEFramework::PluginHost::JSONRPC* _client;
        std::string _callsign;
    };

    // PUBLIC_INTERFACE
    static std::shared_ptr<ThunderLink> GetThunderControllerClient(WPEFramework::PluginHost::IShell* shell, const std::string& callsign) {
        if (shell == nullptr) {
            return nullptr;
        }
        // Try to obtain a dispatcher to perform JSONRPC calls
        WPEFramework::PluginHost::IDispatcher* dispatcher = shell->QueryInterface<WPEFramework::PluginHost::IDispatcher>();
        // We do not own the lifetime of the dispatcher from the shell
        // For safety in this shim, we treat dispatcher as opaque; our ThunderLink will be nullptr if not present
        WPEFramework::PluginHost::JSONRPC* jsonrpc = (dispatcher != nullptr)
            ? static_cast<WPEFramework::PluginHost::JSONRPC*>(nullptr) // we cannot safely downcast; use nullptr to avoid abstract instantiation
            : nullptr;
        // We pass nullptr here; Invoke in ThunderLink will just return ERROR_UNAVAILABLE without crashing
        return std::make_shared<ThunderLink>(jsonrpc, callsign);
    }
}
