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

#include "HttpClient.h"
#include "UtilsLogging.h"

#include <curl/curl.h>
#include <ctime>

#define CURL_TIMEOUT_SEC 10L

namespace WPEFramework
{
    namespace Plugin
    {

        static size_t CurlWriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
        {
            ((std::string *)userp)->append((char *)contents, size * nmemb);
            return size * nmemb;
        }

        long HttpClient::Get(const std::string &url, const std::string &authToken, std::string &response, std::string &responseHeader)
        {
            CURL *curl;
            CURLcode res;
            long retHttpCode = 0;

            if (url.empty())
            {
                LOGERR("Invalid parameters");
                return retHttpCode;
            }

            curl = curl_easy_init();
            if (!curl)
            {
                LOGERR("Failed to initialize curl");
                return retHttpCode;
            }


            CURLcode rc = CURLE_OK;
            if((rc = curl_easy_setopt(curl, CURLOPT_URL, url.c_str())) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_URL) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_TIMEOUT, CURL_TIMEOUT_SEC)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_TIMEOUT) failed: %s", curl_easy_strerror(rc));
            }

            // Create a linked list of custom headers
            struct curl_slist *headers = NULL;
            std::string authorizationParam = "Authorization: Bearer " + authToken;
            headers = curl_slist_append(headers, "Accept: application/json");
            headers = curl_slist_append(headers, "charsets: utf-8");
            headers = curl_slist_append(headers, authorizationParam.c_str());

            if ((rc = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_HTTPHEADER) failed: %s", curl_easy_strerror(rc));
            }

            if ((rc = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_WRITEFUNCTION) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_WRITEDATA) failed: %s", curl_easy_strerror(rc));
            }

            if ((rc = curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, CurlWriteCallback)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_HEADERFUNCTION) failed: %s", curl_easy_strerror(rc));
            }

            if ((rc = curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeader)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_HEADERDATA) failed: %s", curl_easy_strerror(rc));
            }

            // Perform the request, res will get the return code
            res = curl_easy_perform(curl);

            // Check for errors
            if (res != CURLE_OK)
            {
                LOGERR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
            }
            else
            {
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &retHttpCode);
            }

            // Clean up the header list
            curl_slist_free_all(headers);

            // Clean up curl session
            curl_easy_cleanup(curl);

            return retHttpCode;
        }

        long HttpClient::Post(const string &url, const string &authToken, const string &data, std::string &response, std::string &responseHeader)
        {
            CURL *curl;
            CURLcode res;
            long retHttpCode = 0;

            if (url.empty())
            {
                LOGERR("Invalid parameters");
                return retHttpCode;
            }

            curl = curl_easy_init();
            if (!curl)
            {
                LOGERR("Failed to initialize curl");
                return retHttpCode;
            }

            CURLcode rc = CURLE_OK;
            if((rc = curl_easy_setopt(curl, CURLOPT_URL, url.c_str())) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_URL) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_TIMEOUT, CURL_TIMEOUT_SEC)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_TIMEOUT) failed: %s", curl_easy_strerror(rc));
            }

            // Create a linked list of custom headers
            struct curl_slist *headers = NULL;
            std::string authorizationParam = "Authorization: Bearer " + authToken;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, "Accept: application/json");
            headers = curl_slist_append(headers, "charsets: utf-8");
            headers = curl_slist_append(headers, authorizationParam.c_str());

            if ((rc = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_HTTPHEADER) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST")) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_CUSTOMREQUEST) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str())) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_POSTFIELDS) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size())) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_POSTFIELDSIZE) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_WRITEFUNCTION) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_WRITEDATA) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, CurlWriteCallback)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_HEADERFUNCTION) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeader)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_HEADERDATA) failed: %s", curl_easy_strerror(rc));
            }

            // Perform the request, res will get the return code
            res = curl_easy_perform(curl);

            // Check for errors
            if (res != CURLE_OK)
            {
                LOGERR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
            }
            else
            {
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &retHttpCode);
            }

            // Clean up the header list
            curl_slist_free_all(headers);

            // Clean up curl session
            curl_easy_cleanup(curl);

            return retHttpCode;
        }

        long HttpClient::Delete(const std::string &url, const std::string &authToken, std::string &response, std::string &responseHeader)
        {
            CURL *curl;
            CURLcode res;
            long retHttpCode = 0;

            if (url.empty())
            {
                LOGERR("Invalid parameters");
                return retHttpCode;
            }

            curl = curl_easy_init();
            if (!curl)
            {
                LOGERR("Failed to initialize curl");
                return retHttpCode;
            }

            CURLcode rc = CURLE_OK;
            if ((rc = curl_easy_setopt(curl, CURLOPT_URL, url.c_str())) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_URL) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_TIMEOUT, CURL_TIMEOUT_SEC)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_TIMEOUT) failed: %s", curl_easy_strerror(rc));
            }

            // Create a linked list of custom headers
            struct curl_slist *headers = NULL;
            std::string authorizationParam = "Authorization: Bearer " + authToken;
            headers = curl_slist_append(headers, "Accept: application/json");
            headers = curl_slist_append(headers, "charsets: utf-8");
            headers = curl_slist_append(headers, authorizationParam.c_str());

            if ((rc = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_HTTPHEADER) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE")) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_CUSTOMREQUEST) failed: %s", curl_easy_strerror(rc));
            }

            if ((rc = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_WRITEFUNCTION) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_WRITEDATA) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, CurlWriteCallback)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_HEADERFUNCTION) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeader)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_HEADERDATA) failed: %s", curl_easy_strerror(rc));
            }

            // Perform the request, res will get the return code
            res = curl_easy_perform(curl);

            // Check for errors
            if (res != CURLE_OK)
            {
                LOGERR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
            }
            else
            {
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &retHttpCode);
            }

            // Clean up the header list
            curl_slist_free_all(headers);

            // Clean up curl session
            curl_easy_cleanup(curl);

            return retHttpCode;
        }

        long HttpClient::Put(const std::string &url, const std::string &authToken, const std::string &data, std::string &response, std::string &responseHeader)
        {
            CURL *curl;
            CURLcode res;
            long retHttpCode = 0;

            if (url.empty())
            {
                LOGERR("Invalid parameters");
                return retHttpCode;
            }

            curl = curl_easy_init();
            if (!curl)
            {
                LOGERR("Failed to initialize curl");
                return retHttpCode;
            }

            CURLcode rc = CURLE_OK;
            if ((rc = curl_easy_setopt(curl, CURLOPT_URL, url.c_str())) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_URL) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_TIMEOUT, CURL_TIMEOUT_SEC)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_TIMEOUT) failed: %s", curl_easy_strerror(rc));
            }

            // Create a linked list of custom headers
            struct curl_slist *headers = NULL;
            std::string authorizationParam = "Authorization: Bearer " + authToken;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, "Accept: application/json");
            headers = curl_slist_append(headers, "charsets: utf-8");
            headers = curl_slist_append(headers, authorizationParam.c_str());

            if ((rc = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_HTTPHEADER) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT")) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_CUSTOMREQUEST) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str())) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_POSTFIELDS) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size())) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_POSTFIELDSIZE) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_WRITEFUNCTION) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_WRITEDATA) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, CurlWriteCallback)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_HEADERFUNCTION) failed: %s", curl_easy_strerror(rc));
            }
            if ((rc = curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeader)) != CURLE_OK) {
                LOGERR("curl_easy_setopt(CURLOPT_HEADERDATA) failed: %s", curl_easy_strerror(rc));
            }

            // Perform the request, res will get the return code
            res = curl_easy_perform(curl);

            // Check for errors
            if (res != CURLE_OK)
            {
                LOGERR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
            }
            else
            {
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &retHttpCode);
            }

            // Clean up the header list
            curl_slist_free_all(headers);

            // Clean up curl session
            curl_easy_cleanup(curl);

            return retHttpCode;
        }

    }
}
