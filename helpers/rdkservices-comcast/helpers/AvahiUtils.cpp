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

#include "utils.h"
#include "AvahiUtils.h"
#include <net/if.h>

#include <avahi-common/error.h>

namespace WPEFramework  {
namespace Plugin        {

AvahiUtils::AvahiUtils() : avahiPoll(nullptr), avahiClient(nullptr), avahiGroup(nullptr), avahiBrowser(nullptr), browserListener(nullptr)
{
    LOGINFO();

    createAvahiClient();
}

AvahiUtils::~AvahiUtils()
{
    LOGINFO();

    stopAvahiClient();
    destroyAvahiClient();
}

void AvahiUtils::createAvahiClient()
{
    LOGINFO();

    if (!avahiPoll && !avahiClient)
    {
        avahiPoll = avahi_threaded_poll_new();
        running = true;
        if (avahiPoll)
        {
            int error = 0;
            avahiClient = avahi_client_new
                (avahi_threaded_poll_get(avahiPoll), static_cast<AvahiClientFlags>(0), clientCallback, nullptr, &error);
            if (!avahiClient)
            {
                LOGERR("Failed to create avahi client: %s\n", avahi_strerror(error));
            }
        }
        else
        {
            LOGERR("Failed to create avahi poll");
        }
    }
    else
    {
        LOGERR("Avahi poll:client error %p:%p", avahiPoll, avahiClient);
    }
}

bool AvahiUtils::startAvahiClient()
{
    LOGINFO();
    bool success = false;

    if (avahiPoll)
    {
        int error = avahi_threaded_poll_start(avahiPoll);
        if (error >= 0)
        {
            success = true;
        }
        else
        {
            LOGERR("Failed to start avahi poll %s\n", avahi_strerror(error));
        }
    }
    else
    {
        LOGERR("Avahi poll error %p", avahiPoll);
    }

    return success;
}

void AvahiUtils::stopAvahiClient(bool internal)
{
    LOGINFO();

    if (avahiPoll && running)
    {
        if (internal)
        {
            LOGINFO("Quiting thread internal begin");
            avahi_threaded_poll_quit(avahiPoll);
            LOGINFO("Quiting thread internal end");
        }
        else
        {
            avahi_threaded_poll_stop(avahiPoll);
        }
        running = false;
    }
}

boost::optional<AvahiIfIndex> AvahiUtils::getIfIndex(const NetworkInterface& interface)
{
    switch (interface)
    {
        case NetworkInterface::All:
        {
            LOGINFO("Returning AVAHI_IF_UNSPEC");
            return AVAHI_IF_UNSPEC;
        }
        case NetworkInterface::Usb:
        {
            const auto& index = if_nametoindex("usb0");
            if (0 == index)
            {
                LOGERR("Unable to resolve usb0 to interface index.");
                return boost::none;
            }
            LOGINFO("Found index corresponding to usb0 interface: %d", index);
            return index;
        }
    }
    return boost::none;
}

void AvahiUtils::destroyAvahiClient()
{
    LOGINFO();
    if (avahiClient)
    {
        avahi_client_free(avahiClient);  // From docs: This will automatically free all associated browser, resolve and entry group objects.
        avahiClient = nullptr;
        avahiGroup = nullptr;
        avahiBrowser = nullptr;
    }

    if (avahiPoll)
    {
        avahi_threaded_poll_free(avahiPoll);
        avahiPoll = nullptr;
    }
}

bool AvahiUtils::registerAvahiService(std::string name, std::string type, uint16_t port, const AvahiIfIndex& ifIndex)
{
    LOGINFO();
    bool success = false;

    if (avahiClient && !avahiGroup)
    {
        success = addAvahiService(name, type, port, ifIndex);
    }
    else
    {
        LOGERR("Avahi client:group error %p:%p", avahiClient, avahiGroup);
    }

    return success;
}

bool AvahiUtils::addAvahiService(std::string name, std::string type, uint16_t port, const AvahiIfIndex& ifIndex)
{
    LOGINFO();
    bool success = false;

    if (!name.empty() && !type.empty())
    {
        avahiGroup = avahi_entry_group_new(avahiClient, groupCallback, nullptr);
        if (avahiGroup)
        {
            LOGINFO("Adding service:%s type:%s port:%u", name.c_str(), type.c_str(), port);
            int error = avahi_entry_group_add_service
                (avahiGroup, ifIndex, AVAHI_PROTO_UNSPEC, static_cast<AvahiPublishFlags>(0)
                , name.c_str(), type.c_str(), nullptr, nullptr, port, NULL);
            if (error >= 0)
            {
                success = commitAvahiGroup();
            }
            else
            {
                LOGERR("Failed to add service: %s\n", avahi_strerror(error));
            }
        }
        else
        {
            LOGERR("Failed to create avahi group");
        }
    }
    else
    {
        LOGERR("Avahi service name:type error %s:%s", name.c_str(), type.c_str());
    }

    return success;
}

bool AvahiUtils::commitAvahiGroup()
{
    LOGINFO();
    bool success = false;

    if (avahiGroup)
    {
        int error = avahi_entry_group_commit(avahiGroup);
        if (error >= 0)
        {
            success = true;
        }
        else
        {
            LOGERR("Failed to commit entry group: %s\n", avahi_strerror(error));
        }
    }
    else
    {
        LOGERR("Avahi group error %p", avahiGroup);
    }

    return success;
}

bool AvahiUtils::registerAvahiBrowser(std::string type, IAvahiBrowserListener* listener, const AvahiIfIndex& ifIndex)
{
    LOGINFO();
    bool success = false;

    if (avahiClient && !avahiBrowser)
    {
        if (!type.empty())
        {
            avahiBrowser = avahi_service_browser_new
                (avahiClient, ifIndex, AVAHI_PROTO_INET, type.c_str(), nullptr
                , static_cast<AvahiLookupFlags>(0), browseCallback, static_cast<void*>(this));
            if (!avahiBrowser)
            {
                LOGERR("Failed to create avahi browser: %s", avahi_strerror(avahi_client_errno(avahiClient)));
            }
            else
            {
                browserListener = listener;
                success = true;
            }
        }
        else
        {
            LOGERR("Avahi browser type error %s", type.c_str());
        }
    }
    else
    {
        LOGERR("Avahi client:browser error %p:%p", avahiClient, avahiBrowser);
    }

    return success;
}

void AvahiUtils::clientCallback(AvahiClient* client, AvahiClientState state, AVAHI_GCC_UNUSED void* userData)
{
    switch (state)
    {
        case AVAHI_CLIENT_S_RUNNING:
        {
            LOGINFO("Avahi client state AVAHI_CLIENT_S_RUNNING");
            break;
        }
        case AVAHI_CLIENT_FAILURE:
        {
            LOGERR("Avahi client state AVAHI_CLIENT_FAILURE: %s\n", avahi_strerror(avahi_client_errno(client)));
            break;
        }
        case AVAHI_CLIENT_S_COLLISION:
        {
            LOGERR("Avahi client state AVAHI_CLIENT_S_COLLISION");
            break;
        }
        case AVAHI_CLIENT_S_REGISTERING:
        {
            LOGINFO("Avahi client state AVAHI_CLIENT_S_REGISTERING");
            break;
        }
        case AVAHI_CLIENT_CONNECTING:
        {
            LOGINFO("Avahi client state AVAHI_CLIENT_CONNECTING");
            break;
        }
        default:
        {
            LOGINFO();
            break;
        }
    }
}

void AvahiUtils::groupCallback(AvahiEntryGroup* group, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void* userData)
{
    switch (state)
    {
        case AVAHI_ENTRY_GROUP_ESTABLISHED:
        {
            LOGINFO("Avahi group state AVAHI_ENTRY_GROUP_ESTABLISHED");
            break;
        }
        case AVAHI_ENTRY_GROUP_COLLISION:
        {
            LOGERR("Avahi group state AVAHI_ENTRY_GROUP_COLLISION");
            break;
        }
        case AVAHI_ENTRY_GROUP_FAILURE:
        {
            LOGERR("Avahi group state AVAHI_ENTRY_GROUP_FAILURE: %s\n"
                  , avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(group))));
            break;
        }
        case AVAHI_ENTRY_GROUP_UNCOMMITED:
        {
            LOGINFO("Avahi group state AVAHI_ENTRY_GROUP_UNCOMMITED");
            break;
        }
        case AVAHI_ENTRY_GROUP_REGISTERING:
        {
            LOGINFO("Avahi group state AVAHI_ENTRY_GROUP_REGISTERING");
            break;
        }
        default:
        {
            LOGINFO();
            break;
        }
    }
}

void AvahiUtils::browseCallback(AvahiServiceBrowser* browser, AvahiIfIndex interface, AvahiProtocol protocol
    , AvahiBrowserEvent event, const char* name, const char* type, const char* domain
    , AVAHI_GCC_UNUSED AvahiLookupResultFlags flags, void* userData)
{
    switch (event)
    {
        case AVAHI_BROWSER_FAILURE:
        {
            LOGERR("Avahi browse event AVAHI_BROWSER_FAILURE: %s"
                  , avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(browser))));
            break;
        }
        case AVAHI_BROWSER_NEW:
        {
            LOGINFO("Avahi browse event AVAHI_BROWSER_NEW service:type:domain %s:%s:%s", name, type, domain);

            AvahiUtils* avahiUtils = static_cast<AvahiUtils*>(userData);
            if (avahiUtils && avahiUtils->avahiClient)
            {
                if (!(avahi_service_resolver_new(avahiUtils->avahiClient, interface, protocol, name, type, domain
                   , AVAHI_PROTO_UNSPEC, static_cast<AvahiLookupFlags>(0), resolveCallback, userData)))
                {
                    LOGERR("Failed to create avahi resolver name:type %s:%s %s", name, type, avahi_strerror(avahi_client_errno(avahiUtils->avahiClient)));
                }
            }
            break;
        }
        case AVAHI_BROWSER_REMOVE:
        {
            LOGINFO("Avahi browse event AVAHI_BROWSER_REMOVE service:type:domain %s:%s:%s", name, type, domain);

            AvahiUtils* avahiUtils = static_cast<AvahiUtils*>(userData);
            if (avahiUtils && avahiUtils->browserListener)
            {
                avahiUtils->browserListener->onServiceRemove(name, type, domain);
            }

            break;
        }
        case AVAHI_BROWSER_ALL_FOR_NOW:
        {
            LOGINFO("Avahi browse event AVAHI_BROWSER_ALL_FOR_NOW");
            break;
        }
        case AVAHI_BROWSER_CACHE_EXHAUSTED:
        {
            LOGINFO("Avahi browse event AVAHI_BROWSER_CACHE_EXHAUSTED");
            break;
        }
        default:
        {
            LOGINFO();
            break;
        }
    }
}

void AvahiUtils::resolveCallback(AvahiServiceResolver* resolver, AVAHI_GCC_UNUSED AvahiIfIndex interface
    , AVAHI_GCC_UNUSED AvahiProtocol protocol, AvahiResolverEvent event, const char* name, const char* type
    , const char* domain, const char* hostName, const AvahiAddress* address, uint16_t port
    , AvahiStringList* txt, AvahiLookupResultFlags flags, AVAHI_GCC_UNUSED void* userData)
{
    switch (event)
    {
        case AVAHI_RESOLVER_FAILURE:
        {
            LOGERR("Avahi resolve event AVAHI_RESOLVER_FAILURE service:type:domain %s:%s:%s %s"
                  , name, type, domain, avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(resolver))));
            AvahiUtils* avahiUtils = static_cast<AvahiUtils*>(userData);
            if (avahiUtils && avahiUtils->browserListener)
            {
                avahiUtils->browserListener->onServiceResolveFailure(name, type, domain);
            }
            break;
        }
        case AVAHI_RESOLVER_FOUND:
        {
            char ipAddress[AVAHI_ADDRESS_STR_MAX];
            avahi_address_snprint(ipAddress, sizeof(ipAddress), address);
            LOGINFO("Avahi resolve event AVAHI_RESOLVER_FOUND service:type:domain:ip:port %s:%s:%s:%s:%u", name, type, domain, ipAddress, port);

            AvahiUtils* avahiUtils = static_cast<AvahiUtils*>(userData);
            if (avahiUtils && avahiUtils->browserListener)
            {
                avahiUtils->browserListener->onServiceNew(name, type, domain, ipAddress, port);
            }

            break;
        }
        default:
        {
            LOGINFO();
            break;
        }
    }
}

}
}
