/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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

#include <string>
#include <cstdint>

namespace WPEFramework
{
    namespace Utils
    {
        std::string Base64Encode(const void *data, std::size_t len, bool urlSafe = false)
        {
            static const char *B64_STD = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            static const char *B64_URL = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
            const char *tbl = urlSafe ? B64_URL : B64_STD;

            if (len == 0 || data == nullptr)
                return std::string();

            const uint8_t *bytes = static_cast<const uint8_t *>(data);
            std::string out;
            out.resize(4 * ((len + 2) / 3)); // exact output size with padding

            std::size_t i = 0, o = 0;

            // full 3-byte chunks
            while (i + 3 <= len)
            {
                uint32_t n = (uint32_t(bytes[i]) << 16) |
                             (uint32_t(bytes[i + 1]) << 8) |
                             uint32_t(bytes[i + 2]);
                out[o++] = tbl[(n >> 18) & 0x3F];
                out[o++] = tbl[(n >> 12) & 0x3F];
                out[o++] = tbl[(n >> 6) & 0x3F];
                out[o++] = tbl[n & 0x3F];
                i += 3;
            }

            // tail (1 or 2 bytes)
            std::size_t rem = len - i;
            if (rem)
            {
                uint32_t n = uint32_t(bytes[i]) << 16;
                if (rem == 2)
                    n |= uint32_t(bytes[i + 1]) << 8;

                out[o++] = tbl[(n >> 18) & 0x3F];
                out[o++] = tbl[(n >> 12) & 0x3F];

                if (rem == 2)
                {
                    out[o++] = tbl[(n >> 6) & 0x3F];
                    out[o++] = '=';
                }
                else
                { // rem == 1
                    out[o++] = '=';
                    out[o++] = '=';
                }
            }

            return out; // fully filled
        }

        // Convenience overload for std::string
        std::string Base64Encode(const std::string &s, bool urlSafe = false)
        {
            return Base64Encode(s.data(), s.size(), urlSafe);
        }

    }
}