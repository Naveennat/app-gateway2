#pragma once
/*
 * Minimal stub for "WsManager.h" used by the AppGateway plugin in this
 * repository's isolated build.
 *
 * The upstream WsManager provides websocket connection management. For this
 * repository's L0 build verification, we only need enough surface area to
 * compile.
 */
#include <cstdint>
#include <string>

class WsManager {
public:
    WsManager() = default;
    ~WsManager() = default;

    bool Start(const std::string& /*endpoint*/) { return true; }
    void Stop() {}

    bool IsConnected() const { return true; }

    bool Send(const std::string& /*payload*/) { return true; }
    void Close(uint32_t /*connectionId*/ = 0) {}
};
