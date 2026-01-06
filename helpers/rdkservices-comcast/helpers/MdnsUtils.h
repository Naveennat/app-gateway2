/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2023 RDK Management
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
#include <atomic>
#include <boost/optional.hpp>
#include <dns_sd.h>
#include <thread>

namespace WPEFramework  {
namespace Plugin        {

class IMdnsBrowserListener
{
public:
    virtual void onServiceNew(std::string name, std::string type, std::string domain, std::string ip, uint16_t port) = 0;
    virtual void onServiceRemove(std::string name, std::string type, std::string domain) = 0;
    virtual void onServiceResolveFailure(std::string name, std::string type, std::string domain) = 0;
    virtual ~IMdnsBrowserListener() = default;
};

class MdnsUtils
{
public:
    MdnsUtils();
    ~MdnsUtils();
    bool registerService(const std::string& name, const std::string& type, uint16_t port, const std::string& ifName);
    bool registerBrowser(const std::string& type, IMdnsBrowserListener* listener, const std::string& ifName);
    void stopBrowser();

private:
    MdnsUtils(const MdnsUtils&) = delete;
    MdnsUtils& operator=(const MdnsUtils&) = delete;

    void browseThread(const std::string& type, const int interfaceIndex);
    std::atomic_bool browseThreadRunning_{false};
    std::thread browseThread_;
    int interruptFd_[2];

    DNSServiceRef serviceRef_;

    IMdnsBrowserListener* browserListener_;
    std::string foundName_;
    std::string foundType_;
    std::string foundDomain_;
    uint16_t foundPort_;

    static void DNSSD_API getAddrInfoCallback(
        DNSServiceRef sdRef,
        DNSServiceFlags flags,
        uint32_t interfaceIndex,
        DNSServiceErrorType errorCode,
        const char *hostname,
        const struct sockaddr *address,
        uint32_t ttl,
        void *context);

    static void DNSSD_API resolveCallback(
        DNSServiceRef sdRef,
        DNSServiceFlags flags,
        uint32_t interfaceIndex,
        DNSServiceErrorType errorCode,
        const char *fullname,
        const char *hosttarget,
        uint16_t port,
        uint16_t txtLen,
        const unsigned char *txtRecord,
        void *context);

    static void DNSSD_API browseCallback(
        DNSServiceRef sdRef,
        DNSServiceFlags flags,
        uint32_t interfaceIndex,
        DNSServiceErrorType errorCode,
        const char *serviceName,
        const char *regtype,
        const char *replyDomain,
        void *context);


};

}
}
