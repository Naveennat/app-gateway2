#pragma once

/*
 * Minimal stub for Exchange::IAnalytics to satisfy compilation.
 * Provides the SendEvent signature used by MetricsDelegate and FbMetrics.
 */

#include <plugins/Module.h>
#include <com/IIteratorType.h>
#include <com/Ids.h>

#ifndef ID_ANALYTICS
#define ID_ANALYTICS 0xFA300100
#endif

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IAnalytics : virtual public Core::IUnknown {
        enum { ID = ID_ANALYTICS };

        // Alias iterator type for strings used by SendEvent CET list
        using IStringIterator = RPC::IIteratorType<string, RPC::ID_STRINGITERATOR>;

        // PUBLIC_INTERFACE
        virtual Core::hresult SendEvent(const string& eventName /* @in */,
                                        const string& eventVersion /* @in */,
                                        const string& eventSource /* @in */,
                                        const string& eventSourceVersion /* @in */,
                                        IStringIterator* cetList /* @in */,
                                        const uint64_t epochTimestamp /* @in */,
                                        const uint64_t uptime /* @in */,
                                        const string& appId /* @in */,
                                        const string& payload /* @in @opaque */) = 0;
        /** Send an analytics event to the Analytics bridge. */
    };

} // namespace Exchange
} // namespace WPEFramework
