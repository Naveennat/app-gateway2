#pragma once

#include <interfaces/IApp2AppProvider.h>
#include <interfaces/IAppGateway.h>

// PUBLIC_INTERFACE
inline WPEFramework::Exchange::IApp2AppProvider::Context ConvertAppGatewayToProviderContext(
    const WPEFramework::Exchange::GatewayContext& src,
    const string& origin)
{
    WPEFramework::Exchange::IApp2AppProvider::Context result;
    result.requestId = src.requestId;
    result.connectionId = src.connectionId;
    result.appId = src.appId;
    result.origin = origin;
    return result;
}

// PUBLIC_INTERFACE
inline WPEFramework::Exchange::GatewayContext ConvertProviderToAppGatewayContext(
    const WPEFramework::Exchange::IApp2AppProvider::Context& src)
{
    WPEFramework::Exchange::GatewayContext result;
    result.requestId = src.requestId;
    result.connectionId = src.connectionId;
    result.appId = src.appId;
    return result;
}
