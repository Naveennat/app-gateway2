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

#include <cstdint>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include <openssl/x509.h>
#include <openssl/pkcs12.h>
#include <openssl/pem.h>

namespace WPEFramework {
namespace Plugin       {
namespace WebSockets   {

class Certificates
{
public:
    bool prepareCertFiles() const;
    std::string getCertPath() const;
    std::string getKeyPath() const;
    std::vector<std::string> getCertAuthoritiesPaths() const;
    bool removeCertFiles() const;

private:
    bool prepareTempDirectory() const;
    bool generatePemFiles() const;
    bool addCertP12Chain(FILE *cert_key_fp, STACK_OF(X509) *additional_certs) const;
    bool getCertP12Phasephrase(char* pcert_p12_passphrase) const;

    const boost::filesystem::path tmpCertPath_{"/tmp/WebSocketsHelpersCerts_" + std::to_string(uintptr_t(this))};
    const boost::filesystem::path combinedCertPemFile_ {tmpCertPath_ / "combinedCert.pem"};
    const boost::filesystem::path pkcsFileLocation_{"/opt/certs/devicecert_1.pk12"};

    const std::vector<std::string> certificateAuthorities_{
        "/etc/ssl/certs/Xfinity_Subscriber_ECC_Root.pem",
        "/etc/ssl/certs/Xfinity_Subscriber_RSA_Root.pem"
    };
};

} // namespace WebSockets
} // namespace Plugin
} // namespace WPEFramework