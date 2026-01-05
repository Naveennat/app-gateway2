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

#include "Certificates.h"
#include <secure_wrapper.h>

#include "../utils.h"

#define CERT_PASS_MAX_LEN            32

namespace WPEFramework {
namespace Plugin       {
namespace WebSockets   {

bool Certificates::prepareCertFiles() const
{
    LOGINFO("Preparing .pem cert file\n");

    if (!boost::filesystem::exists(pkcsFileLocation_))
    {
        LOGERR("Source pkcs file doesn't exist in: %s\n", pkcsFileLocation_.c_str());
        return false;
    }

    if (!prepareTempDirectory())
    {
        LOGERR("Creating temp directory failed, skipping certificate extraction.\n");
        return false;
    }
    return generatePemFiles();
}

std::string Certificates::getCertPath() const
{
    if (!boost::filesystem::exists(combinedCertPemFile_))
    {
        LOGERR("Certificate files are not initialized.");
        return {};
    }
    return combinedCertPemFile_.native();
}

std::string Certificates::getKeyPath() const
{
    if (!boost::filesystem::exists(combinedCertPemFile_))
    {
        LOGERR("Certificate files are not initialized.");
        return {};
    }
    return combinedCertPemFile_.native();
}

std::vector<std::string> Certificates::getCertAuthoritiesPaths() const
{
    return certificateAuthorities_;
}

bool Certificates::removeCertFiles() const
{
    LOGINFO("Removing temp cert directory: %s\n", tmpCertPath_.c_str());
    try {
        boost::filesystem::remove_all(tmpCertPath_);
    } catch (const std::exception& e) {
        LOGERR("Error while removing old temp cert directory: %s\n", e.what());
        return false;
    }
    return true;
}

bool Certificates::prepareTempDirectory() const
{
    LOGINFO("Preparing temp cert directory in: %s\n", tmpCertPath_.c_str());

    if (boost::filesystem::exists(tmpCertPath_) && !removeCertFiles())
    {
        LOGERR("Removing existing temp cert directory failed.\n");
        return false;
    }

    LOGINFO("Creating temp cert directory\n");
    try {
        boost::filesystem::create_directory(tmpCertPath_);
    } catch (const std::exception& e) {
        LOGERR("Error while creating temp cert directory: %s\n", e.what());
        return false;
    }
    return true;
}

bool Certificates::getCertP12Phasephrase(char* pcert_p12_passphrase) const
{
    FILE *pPassphraseCmdFp = nullptr;
    size_t len = 0;
    int err_store = 0;
    bool ret_value = false;

    if (nullptr == pcert_p12_passphrase)
    {
       LOGERR("pcert_p12_passphrase is null");
    }
    else
    {
        pPassphraseCmdFp = v_secure_popen("r", "/usr/bin/rdkssacli \"{STOR=GET,SRC=kquhqtoczcbx,DST=/dev/stdout}\"");

        if (nullptr != pPassphraseCmdFp)
        {
            while (fgets(pcert_p12_passphrase, CERT_PASS_MAX_LEN, pPassphraseCmdFp) != nullptr);

            len = strlen(pcert_p12_passphrase);

            if (len > 0 && pcert_p12_passphrase[len - 1] == '\n')
            {
                // Remove trailing newline
                pcert_p12_passphrase[len - 1] = '\0';
                ret_value = true;
            }

            v_secure_pclose(pPassphraseCmdFp);
            pPassphraseCmdFp = nullptr;
        }
    }

    return ret_value;
}

bool Certificates::addCertP12Chain(FILE *cert_key_fp, STACK_OF(X509) *additional_certs) const
{
   bool ret_value = false;

   if (cert_key_fp == nullptr)
   {
       LOGERR("null file pointer");
   }
   else if (additional_certs == nullptr)
   {
       LOGERR("null certs");
   }
   else
   {
       uint32_t num_additional_certs = sk_X509_num(additional_certs);
       ret_value = true;

       for (uint32_t index = 0; index < num_additional_certs; index++)
       {
           X509 *cert = sk_X509_value(additional_certs, index);

           if (nullptr == cert)
           {
               LOGERR("null cert index %d", index);
               ret_value = false;
               break;
           }
           else
           {
               if((1 != PEM_write_X509(cert_key_fp, cert)))
               {
                  LOGERR("failed to write temp cert index %d", index);
                  ret_value = false;
                  break;
               }
           }
       }
   }

   return ret_value;
}

bool Certificates::generatePemFiles() const
{
    char cert_p12_passphrase[CERT_PASS_MAX_LEN] = {0};
    FILE *cert_key_fp = nullptr;
    FILE *device_cert_fp = nullptr;
    PKCS12 *p12_cert = nullptr;
    EVP_PKEY *pkey = nullptr;
    X509 *x509_cert = nullptr;
    STACK_OF(X509) *additional_certs = nullptr;
    int err_store = 0;
    bool ret_value = false;

    LOGINFO("Generating %s file\n", combinedCertPemFile_.native().c_str());

    if (nullptr == (cert_key_fp = fopen((char *)combinedCertPemFile_.native().c_str() , "w")))
    {
       err_store = errno;
       LOGERR("fopen failed: <%s>", strerror(err_store));
    }
    else
    {
        // Fetch the certificate passphrase
        if (true == getCertP12Phasephrase(cert_p12_passphrase))
        {
            device_cert_fp = fopen((char *)pkcsFileLocation_.native().c_str() , "rb");

            if(device_cert_fp == nullptr)
            {
                LOGERR("unable to open P12 certificate");
            }
            else
            {
                d2i_PKCS12_fp(device_cert_fp, &p12_cert);
                fclose(device_cert_fp);
                device_cert_fp = nullptr;

                if (p12_cert == nullptr)
                {
                    LOGERR("unable to read P12 certificate");
                }
                else if (1 != PKCS12_parse(p12_cert, cert_p12_passphrase, &pkey, &x509_cert, &additional_certs))
                {
                    LOGERR("unable to parse P12 certificate");
                }
                else if (nullptr == x509_cert)
                {
                    LOGERR("null x509_cert");
                }
                else if (1 != PEM_write_X509(cert_key_fp, x509_cert))
                {
                    LOGERR("failed to write temp cert");
                }
                else if (nullptr == additional_certs)
                {
                    LOGERR("null certs");
                }
                else if (false == addCertP12Chain(cert_key_fp, additional_certs))
                {
                    LOGERR("addCertP12Chain failed");
                }
                else if (1 != PEM_write_PrivateKey(cert_key_fp, pkey, nullptr, (unsigned char*)cert_p12_passphrase, strlen(cert_p12_passphrase), nullptr, nullptr))
                {
                   LOGERR("failed to write temp key");
                }
                else
                {
                    LOGINFO("Pem file created\n");
                    ret_value = true;
                }
           }
        }

        fclose(cert_key_fp);
        cert_key_fp = nullptr;
    }

    return ret_value;
}

} // namespace WebSockets
} // namespace Plugin
} // namespace WPEFramework