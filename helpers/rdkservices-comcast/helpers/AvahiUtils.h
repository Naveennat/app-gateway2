/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2021 RDK Management
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

#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/thread-watch.h>
#include <avahi-client/lookup.h>
#include <boost/optional.hpp>

namespace WPEFramework  {
namespace Plugin        {

class IAvahiBrowserListener
{
public:
    virtual void onServiceNew(std::string name, std::string type, std::string domain, std::string ip, uint16_t port) = 0;
    virtual void onServiceRemove(std::string name, std::string type, std::string domain) = 0;
    virtual void onServiceResolveFailure(std::string name, std::string type, std::string domain) = 0;
    virtual ~IAvahiBrowserListener() = default;
};

class AvahiUtils
{
public:
    AvahiUtils();
    ~AvahiUtils();
    bool startAvahiClient();
    bool registerAvahiService(std::string name, std::string type, uint16_t port, const AvahiIfIndex& ifIndex);
    bool registerAvahiBrowser(std::string type, IAvahiBrowserListener* listener, const AvahiIfIndex& ifIndex);
    // Rationale for flag: Avahi provides two different methods to stop it's event loop
    // depending on calling source: External thread or Event loop thread.
    void stopAvahiClient(bool internal = false);

    enum class NetworkInterface
    {
        Usb,
        All
    };

    static boost::optional<AvahiIfIndex> getIfIndex(const NetworkInterface& interface);

private:
    AvahiUtils(const AvahiUtils&) = delete;
    AvahiUtils& operator=(const AvahiUtils&) = delete;
    static void clientCallback(AvahiClient* client, AvahiClientState state, void* userData);
    static void groupCallback(AvahiEntryGroup* group, AvahiEntryGroupState state, void* userData);
    static void browseCallback(AvahiServiceBrowser* browser, AvahiIfIndex interface, AvahiProtocol protocol
            , AvahiBrowserEvent event, const char* name, const char* type, const char* domain
            , AvahiLookupResultFlags flags, void* userData);
    static void resolveCallback(AvahiServiceResolver* resolver, AVAHI_GCC_UNUSED AvahiIfIndex interface
            , AVAHI_GCC_UNUSED AvahiProtocol protocol, AvahiResolverEvent event, const char* name, const char* type
            , const char* domain, const char* hostName, const AvahiAddress* address, uint16_t port
            , AvahiStringList* txt, AvahiLookupResultFlags flags, AVAHI_GCC_UNUSED void* userData);
    void createAvahiClient();
    void destroyAvahiClient();
    bool addAvahiService(std::string name, std::string type, uint16_t port, const AvahiIfIndex& ifIndex);
    bool commitAvahiGroup();

    bool running{false};
    AvahiThreadedPoll* avahiPoll;
    AvahiClient* avahiClient;
    AvahiEntryGroup* avahiGroup;
    AvahiServiceBrowser* avahiBrowser;
    IAvahiBrowserListener* browserListener;
};

}
}
