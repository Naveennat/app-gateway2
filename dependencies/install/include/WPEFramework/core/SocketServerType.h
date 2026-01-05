#pragma once

/*
 * Minimal shim for WPEFramework::Core::SocketServerType
 *
 * The upstream Thunder Core SDK provides a SocketServerType template used by some
 * websocket/server helpers. The trimmed SDK snapshot shipped with this repository
 * does not include it, but AppGateway's Supporting_Files/WsManager.h depends on it.
 *
 * This shim is intentionally minimal and only aims to satisfy compilation for unit/l0 tests.
 * It does NOT implement a functional socket server.
 */

#include <WPEFramework/core/Proxy.h>
#include <WPEFramework/core/NodeId.h>

namespace WPEFramework {
namespace Core {

    template <typename CLIENT>
    class SocketServerType {
    public:
        SocketServerType()
            : _node()
        {
        }

        // WsManager expects a ctor taking NodeId
        explicit SocketServerType(const Core::NodeId& node)
            : _node(node)
        {
        }

        virtual ~SocketServerType() = default;

        SocketServerType(const SocketServerType&) = delete;
        SocketServerType& operator=(const SocketServerType&) = delete;

        // PUBLIC_INTERFACE
        bool Open(const Core::NodeId& /*node*/)
        {
            /** Open the server on the given node (shim: always succeeds). */
            return true;
        }

        // PUBLIC_INTERFACE
        bool Open(const uint32_t /*timeout*/)
        {
            /** Open with a timeout (shim: always succeeds). */
            return true;
        }

        // PUBLIC_INTERFACE
        void Close()
        {
            /** Close the server (shim: no-op). */
        }

        // PUBLIC_INTERFACE
        void Close(const uint32_t /*code*/)
        {
            /** Close with a status/code (shim: no-op). */
        }

        // PUBLIC_INTERFACE
        void Submit(const uint32_t /*id*/, const Core::ProxyType<Core::JSON::IElement>& /*message*/)
        {
            /** Submit a message to a connected client (shim: no-op). */
        }

        // PUBLIC_INTERFACE
        const CLIENT* Client(const uint32_t /*id*/) const
        {
            /** Return a client pointer (shim: no backing clients available). */
            return nullptr;
        }

        // PUBLIC_INTERFACE
        CLIENT* Client(const uint32_t /*id*/)
        {
            /** Return a client pointer (shim: no backing clients available). */
            return nullptr;
        }

    private:
        Core::NodeId _node;
    };

} // namespace Core
} // namespace WPEFramework
