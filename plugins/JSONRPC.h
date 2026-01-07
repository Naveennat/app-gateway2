#pragma once
/*
 * Minimal shim for <plugins/JSONRPC.h> for isolated compilation.
 *
 * The full Thunder/WPEFramework JSONRPC header pulls in websocket stack headers
 * which are incompatible/unavailable in this isolated build environment.
 *
 * This project only needs the JSONRPC type for inheritance/registration at
 * compile-time; behavior is exercised in L0 via mocks/harness.
 */
#include <cstdint>
#include <string>

namespace WPEFramework {
namespace PluginHost {
class JSONRPC {
public:
    JSONRPC() = default;
    virtual ~JSONRPC() = default;

    // Minimal API surface used by this repo's generated JSON-RPC wrappers.
    template <typename FN>
    void Register(const std::string&, FN&&) {}
    void Unregister(const std::string&) {}
};
} // namespace PluginHost
} // namespace WPEFramework
