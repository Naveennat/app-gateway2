/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2022 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/

#pragma once

#include <functional>
#include <string>
#include <boost/filesystem.hpp>

#include "websocketpp/config/asio_client.hpp"
#include "websocketpp/client.hpp"

#include "../../ScopeGuard.h"
#include "../../utils.h"
#include "../Certificates.h"

namespace WPEFramework {
namespace Plugin       {
namespace WebSockets   {

template <typename Derived, typename Role>
class MTlsEnabled
{
public:
    MTlsEnabled();

protected:
    using EndpointType = typename Role::EncryptedEndpointType;
    using WebsocketppContextPtr = websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context>;

    ~MTlsEnabled();
    void setup();
    std::string addProtocolToAddress(const std::string& address);
    void setAuthenticationState(ConnectionInitializationResult& result, websocketpp::connection_hdl hdl);

private:
    WebsocketppContextPtr onTlsInit(websocketpp::connection_hdl);
    bool logIfCertFailure(bool preverified, boost::asio::ssl::verify_context& verify_ctx) const;
    void loadCertificateAuthorities(const WebsocketppContextPtr& ctx) const;

    const Certificates certificates_;
};

template <typename Derived, typename Role>
MTlsEnabled<Derived, Role>::MTlsEnabled()
{
    LOGINFO("Creating mTLS enabled websocket.\n");
}

template <typename Derived, typename Role>
MTlsEnabled<Derived, Role>::~MTlsEnabled()
{
    LOGINFO("Destroying mTLS enabled websocket.\n");
}

template <typename Derived, typename Role>
void MTlsEnabled<Derived, Role>::setup()
{
    LOGINFO("Setting up TLS handler.\n");
    Derived& derived = static_cast<Derived&>(*this);
    derived.endpointImpl_.set_tls_init_handler(std::bind(&MTlsEnabled<Derived, Role>::onTlsInit, this, std::placeholders::_1));
}

template <typename Derived, typename Role>
std::string MTlsEnabled<Derived, Role>::addProtocolToAddress(const std::string& address)
{
    return std::string("wss://") + address;
}

template <typename Derived, typename Role>
void MTlsEnabled<Derived, Role>::setAuthenticationState(ConnectionInitializationResult& result, websocketpp::connection_hdl handler)
{
    Derived& derived = static_cast<Derived&>(*this);
    const auto& connection = derived.getConnection(handler);

    const auto& errorCategory = websocketpp::transport::asio::socket::get_socket_category();
    if (errorCategory == connection->get_ec().category())
    {
        using namespace websocketpp::transport::asio::socket;
        switch (connection->get_ec().value())
        {
            case error::value::security:
            case error::value::invalid_tls_context:
            case error::value::tls_handshake_timeout:
            case error::value::missing_tls_init_handler:
            case error::value::tls_handshake_failed:
            case error::value::tls_failed_sni_hostname:
                LOGINFO("TLS connection not established.");
                result.setAuthenticationSuccess(false);
                return;
            break;
            default:
            break;
        }
    }
    result.setAuthenticationSuccess(true);
}

template <typename Derived, typename Role>
typename MTlsEnabled<Derived, Role>::WebsocketppContextPtr MTlsEnabled<Derived, Role>::onTlsInit(websocketpp::connection_hdl)
{
    LOGINFO("Establishing TLS context.\n");
    WebsocketppContextPtr ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

    ScopeGuard<std::function<void()> > guard{[this] () { certificates_.removeCertFiles(); }};

    if (certificates_.prepareCertFiles())
    {
        try {
            LOGINFO("Loading certificates to context.\n");
            ctx->set_options(boost::asio::ssl::context::default_workarounds |
                boost::asio::ssl::context::no_sslv2 |
                boost::asio::ssl::context::no_sslv3 |
                boost::asio::ssl::context::no_tlsv1 |
                boost::asio::ssl::context::no_tlsv1_1 |
                boost::asio::ssl::context::single_dh_use);

            LOGINFO("Setting verify mode to verify peer.\n");
            ctx->set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::context::verify_fail_if_no_peer_cert);
            ctx->set_verify_callback(
                std::bind(&MTlsEnabled::logIfCertFailure, this, std::placeholders::_1, std::placeholders::_2));

            loadCertificateAuthorities(ctx);

            const auto& certFile = certificates_.getCertPath();
            if (certFile.empty())
            {
                LOGERR("Cert file not initialized properly.");
                return ctx;
            }
            LOGINFO("Loading certificate chain from: %s\n", certFile.c_str());
            ctx->use_certificate_chain_file(certFile);

            const auto& keyFile = certificates_.getKeyPath();
            if (keyFile.empty())
            {
                LOGERR("Cert file not initialized properly.");
                return ctx;
            }
            LOGINFO("Loading private key from: %s\n", keyFile.c_str());
            ctx->use_private_key_file(keyFile, boost::asio::ssl::context::pem);

            LOGINFO("Enabling advanced cipher negotiation.");
            SSL_CTX_set_ecdh_auto(ctx->native_handle(), 1);

        } catch (const std::exception& e) {
            LOGERR("Error in context pointer: %s\n", e.what());
        }
    }

    return ctx;
};

template <typename Derived, typename Role>
bool MTlsEnabled<Derived, Role>::logIfCertFailure(bool preverified, boost::asio::ssl::verify_context& verify_ctx) const
{
    if (!preverified)
    {
        std::string errstr(X509_verify_cert_error_string(X509_STORE_CTX_get_error(verify_ctx.native_handle())));
        LOGERR("Certificate verification failed: %s\n", errstr.c_str());
    }
    return preverified;
}

template <typename Derived, typename Role>
void MTlsEnabled<Derived, Role>::loadCertificateAuthorities(const WebsocketppContextPtr& ctx) const
{
    for (const auto& certificateAuthority : certificates_.getCertAuthoritiesPaths())
    {
        LOGINFO("Loading Certificate Authority from: %s\n", certificateAuthority.c_str());
        if (!boost::filesystem::exists(certificateAuthority))
        {
            LOGWARN("Certificate: %s doesn't exist.\n", certificateAuthority.c_str());
            continue;
        }
        try {
            ctx->load_verify_file(certificateAuthority);
        } catch (const std::exception& e) {
            LOGERR("Error while loading certificate: %s, message: %s\n", certificateAuthority.c_str(), e.what());
        }
    }
}

}   // namespace Websockets
}   // namespace Plugin
}   // namespace WPEFramework