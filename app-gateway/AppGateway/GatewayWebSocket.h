#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

/**
 * GatewayWebSocket
 * Placeholder for a server-side WebSocket acceptor/producer to multiplex app connections.
 * For Phase 1 inside this repository, it remains a stub (no server is spun up).
 */
class GatewayWebSocket {
public:
    GatewayWebSocket()
        : _running(false)
        , _port(0) {}

    bool Start(const uint16_t port, string& error) {
        if (_running) {
            error = "Already running";
            return false;
        }
        _running = true;
        _port = port;
        return true;
    }

    void Stop() {
        _running = false;
        _port = 0;
    }

    bool SendTo(const string& connectionId, const string& json) {
        (void)connectionId;
        (void)json;
        // No-op stub
        return true;
    }

    bool Running() const { return _running; }
    uint16_t Port() const { return _port; }

private:
    std::atomic<bool> _running;
    uint16_t _port;
};

} // namespace Plugin
} // namespace WPEFramework
