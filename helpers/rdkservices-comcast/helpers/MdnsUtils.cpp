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

#include "utils.h"
#include "MdnsUtils.h"
#include <dns_sd.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/select.h>

namespace WPEFramework  {
namespace Plugin        {

MdnsUtils::MdnsUtils() : interruptFd_{-1, -1}, browseThreadRunning_(false), serviceRef_(NULL)
{
    LOGINFO();
}

MdnsUtils::~MdnsUtils()
{
    LOGINFO("browseThreadRunning_: %s interruptFd_: %d %d", browseThreadRunning_ ? "true":"false",
            interruptFd_[0], interruptFd_[1]);

    if(browseThreadRunning_ && (interruptFd_[1] > 0))
    {
        stopBrowser();
    }

    if(interruptFd_[0] != -1)
    {
        close(interruptFd_[0]);
    }

    if(interruptFd_[1] != -1)
    {
        close(interruptFd_[1]);
    }

    if(serviceRef_ != NULL)
    {
        LOGINFO("Stopping advertisement");
        DNSServiceRefDeallocate(serviceRef_);
        serviceRef_ = NULL;
    }
}

void MdnsUtils::stopBrowser()
{
    if(!(browseThreadRunning_ && (interruptFd_[1] > 0)))
        return;
    write(interruptFd_[1], "x", 1);
    if(browseThread_.joinable())
    {
        LOGINFO("Waiting for browseThread to finish");
        browseThread_.join();
        LOGINFO("BrowseThread finished");
    }
    LOGINFO("Browser stopped");
}

bool MdnsUtils::registerService(const std::string& name, const std::string& type, uint16_t port, const std::string& ifName)
{
    LOGINFO();
    unsigned int interfaceIndex = kDNSServiceInterfaceIndexAny;
    bool result = false;

    if(ifName == "ANY")
    {
        LOGWARN("Will advertise on _ALL_ network interfaces");
    }
    else
    {
        interfaceIndex = if_nametoindex(ifName.c_str());

        if(interfaceIndex == 0)
        {
            LOGERR("Interface %s not found - not registering", ifName.c_str());
            return result;
        }
    }

    if(serviceRef_ != NULL)
    {
        LOGWARN("Replacing previous advertiser");
        DNSServiceRefDeallocate(serviceRef_);
        serviceRef_ = NULL;
    }

    DNSServiceErrorType err = DNSServiceRegister(
        &serviceRef_,
        0,
        interfaceIndex,
        name.c_str(),
        type.c_str(),
        NULL,
        NULL,
        htons(port),
        0,
        NULL,
        NULL,
        NULL);

    if(err != 0)
    {
        LOGERR("Error %d registering service", err);
    }

    return (err == 0);
}

bool MdnsUtils::registerBrowser(const std::string& type, IMdnsBrowserListener* listener, const std::string& ifName)
{
    LOGINFO();
    bool result = false;
    unsigned int interfaceIndex = kDNSServiceInterfaceIndexAny;

    if(listener == NULL)
    {
        LOGERR("No listener provided");
        return result;
    }

    if(ifName == "ANY")
    {
        LOGWARN("Will browse _ALL_ network interfaces");
    }
    else
    {
        interfaceIndex = if_nametoindex(ifName.c_str());

        if(interfaceIndex == 0)
        {
            LOGERR("Interface %s not found - not registering", ifName.c_str());
            return result;
        }
    }
    browserListener_ = listener;

    LOGINFO("Starting MDNS browse thread");
    browseThread_ = std::thread(&MdnsUtils::browseThread, this, type, interfaceIndex);

    result = true;

    return result;
}

void MdnsUtils::browseThread(const std::string& type, const int interfaceIndex)
{
    if(browseThreadRunning_)
    {
        LOGERR("BrowseThread alread running?!");
        return;
    }

    if(pipe(interruptFd_) == -1)
    {
      LOGERR("Error creating pipe.");
      return;
    }

    DNSServiceRef browserRef;
    DNSServiceErrorType error = DNSServiceBrowse(&browserRef, 0, interfaceIndex, type.c_str(), NULL, browseCallback, this);
    if (error != kDNSServiceErr_NoError)
    {
        LOGERR("Error starting service discovery: %d\n", error);
        return;
    }

    int serviceFd = DNSServiceRefSockFD(browserRef);

    browseThreadRunning_ = true;
    while(browseThreadRunning_)
    {
        fd_set rfds;

        FD_ZERO(&rfds);
        FD_SET(serviceFd, &rfds);
        FD_SET(interruptFd_[0], &rfds);

        LOGINFO("Waiting for DNSServiceBrowse\n");
        int retval = select(
            max(interruptFd_[0], serviceFd)+1,
            &rfds, NULL, NULL, NULL);

        LOGINFO("Select returned");

        if(retval == -1) {
            LOGERR("Error on select - quitting!\n");
            browseThreadRunning_ = false;
            break;
        }

        if(FD_ISSET(serviceFd, &rfds))
        {
            LOGINFO("DNSServiceRef set - process result");
            DNSServiceProcessResult(browserRef);
        }
        else if(FD_ISSET(interruptFd_[0], &rfds))
        {
            LOGINFO("Exiting browseThread due to interrupt pipe being set");
            char buffer[10];
            read(interruptFd_[0], buffer, sizeof(buffer));
            browseThreadRunning_ = false;
            close(interruptFd_[0]);
            interruptFd_[0] = -1;
            close(interruptFd_[1]);
            interruptFd_[1] = -1;
        }
        else
        {
            LOGWARN("No FDs set?!");
        }
    }

    DNSServiceRefDeallocate(browserRef);

    LOGINFO("browseThread exited\n");
    return;
}

void MdnsUtils::DNSSD_API getAddrInfoCallback(
    DNSServiceRef sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    DNSServiceErrorType errorCode,
    const char *hostname,
    const struct sockaddr *address,
    uint32_t ttl,
    void *context)
{
    if (errorCode == kDNSServiceErr_NoError) {
      if (address && address->sa_family ==  AF_INET) {
        char ip[256];
        const unsigned char *b = (const unsigned char *) &((struct sockaddr_in *)address)->sin_addr;
        snprintf(ip, sizeof(ip), "%d.%d.%d.%d", b[0], b[1], b[2], b[3]);
        LOGINFO("IPv4 address : %s\n", ip);
        MdnsUtils *owner = ((MdnsUtils *) context);
        owner->browserListener_->onServiceNew(
            owner->foundName_,
            owner->foundType_,
            owner->foundDomain_,
            ip,
            owner->foundPort_
            );
      } else {
        LOGINFO("Not an IPv4 address!!");
      }
    }
}

void MdnsUtils::DNSSD_API resolveCallback(
    DNSServiceRef sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    DNSServiceErrorType errorCode,
    const char *fullname,
    const char *hosttarget,
    uint16_t port,
    uint16_t txtLen,
    const unsigned char *txtRecord,
    void *context)
{
    MdnsUtils *owner = ((MdnsUtils *) context);

    if (errorCode == kDNSServiceErr_NoError) {
      owner->foundPort_ = ntohs(port);
      LOGINFO("Found %s at %s:%d\n", fullname, hosttarget, owner->foundPort_);

      LOGINFO("Resolving address of %s\n", hosttarget);
      DNSServiceRef addrInfoRef;
      DNSServiceErrorType getAddrInfoError = DNSServiceGetAddrInfo(&addrInfoRef, 0, interfaceIndex, kDNSServiceProtocol_IPv4, hosttarget, getAddrInfoCallback, context);

      if (getAddrInfoError == kDNSServiceErr_NoError) {
        DNSServiceProcessResult(addrInfoRef);
      } else {
        LOGERR("Error resolving service: %d\n", getAddrInfoError);
      }
      DNSServiceRefDeallocate(addrInfoRef);
    }
    else
    {
        LOGINFO("Error in service discovery: %d\n", errorCode);
        owner->browserListener_->onServiceResolveFailure(
            owner->foundName_,
            owner->foundType_,
            owner->foundDomain_);
    }
}

void DNSSD_API MdnsUtils::browseCallback(
    DNSServiceRef sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    DNSServiceErrorType errorCode,
    const char *serviceName,
    const char *regtype,
    const char *replyDomain,
    void *context)
{
    MdnsUtils *owner = ((MdnsUtils *) context);

    if (errorCode != kDNSServiceErr_NoError) {
        LOGINFO("Error in service discovery: %d\n", errorCode);
        owner->browserListener_->onServiceResolveFailure(
            serviceName,
            regtype,
            replyDomain);
        return;
    }

    if(flags & kDNSServiceFlagsAdd)
    {
        LOGINFO("Service added: %s\n", serviceName);

        owner->foundName_ = serviceName;
        owner->foundDomain_ = replyDomain;
        owner->foundType_ = regtype;

        DNSServiceRef resolveRef;
        DNSServiceErrorType resolveError = DNSServiceResolve(&resolveRef, 0, interfaceIndex, serviceName, regtype, replyDomain, resolveCallback, context);

        if (resolveError == kDNSServiceErr_NoError) {
            DNSServiceProcessResult(resolveRef);
        } else {
            LOGINFO("Error resolving service: %d\n", resolveError);
        }
        DNSServiceRefDeallocate(resolveRef);
    }
    else
    {
        LOGINFO("Service removed: %s\n", serviceName);
        owner->browserListener_->onServiceRemove(
            serviceName,
            regtype,
            replyDomain);
    }
}

}
}
