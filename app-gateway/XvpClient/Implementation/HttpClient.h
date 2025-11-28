/*
 * Copyright 2023 Comcast Cable Communications Management, LLC
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
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

    class HttpClient {
    public:
        HttpClient() = default;
        ~HttpClient() = default;

        static long Get(const std::string& url, const std::string& authToken, std::string& response, std::string& responseHeader);
        static long Post(const string &url, const string &authToken, const string &data, std::string &response, std::string &responseHeader);
        static long Delete(const std::string& url, const std::string& authToken, std::string& response, std::string& responseHeader);
        static long Put(const std::string& url, const std::string& authToken, const std::string& data, std::string& response, std::string& responseHeader);

    };
}
}