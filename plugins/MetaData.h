#pragma once
/*
 * Shim for <plugins/MetaData.h> to avoid SDK interface mismatches in isolated build.
 *
 * The real Thunder MetaData interface differs across versions; AppGateway L0 build
 * does not require runtime plugin registry metadata, so provide a minimal stub.
 */
#include <cstdint>
#include <string>

namespace WPEFramework {
namespace Plugin {

template <typename ACTUALSERVICE>
class Metadata {
public:
    Metadata(uint8_t, uint8_t, uint8_t, const std::string&, const std::string&,
             std::initializer_list<const char*> = {},
             std::initializer_list<const char*> = {},
             std::initializer_list<const char*> = {})
    {
    }
};

} // namespace Plugin
} // namespace WPEFramework
