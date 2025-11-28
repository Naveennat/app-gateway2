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

namespace WPEFramework {
namespace Plugin {
/**
 * @brief Get the ID of the foreground app.
 *
 * This function retrieves the list of running apps and filters out the foreground app.
 * The foreground app is determined by the "fireboltStatus" being "FOREGROUND" and "status" being "RUNNING".
 *
 * @return The ID of the foreground app if found, otherwise an empty string.
 */
const std::string getHostForegroupAppID();

/**
 * @brief Check if the specified app is installed and is a Cherry app.
 *
 * This function retrieves the list of installed apps and checks if the specified app is installed.
 * The app is considered a Cherry app if the "cherryApp" attribute is true.
 *
 * @param appID The ID of the app to check.
 * @return True if the specified app is installed and is a Cherry app, otherwise false.
 */
bool checkHostInstalledAppIsCherry(const std::string &appID);

/**
 * @brief Formulates an AMP launch request and sends it to the AS.
 *
 * @param appID The ID of the app to launch.
 * @param appContentData The appContentData to pass to the AMP.
 */
bool launchAMP(const std::string &appID, const std::string &appContentData);

/**
 * @brief Get an app token from the persistent store for a specific app.
 *
 * This function retrieves the value of a specific token from the persistent store for a specific app.
 *
 * @param appID The ID of the app.
 * @param key The key to retrieve the value for.
 * @param scope The scope to retrieve the key value from.
 * @return The value of the key if found, otherwise an empty string.
 */
std::string getAppToken(const std::string &appID, const std::string &key, const std::string &scope);

/**
 * @brief Set an app token in the persistent store for a specific app.
 *
 * This function sets the value of a specific token in the persistent store for a specific app.
 *
 * @param appID The ID of the app.
 * @param key The key to set the value for.
 * @param value The value to set.
 * @param scope The scope to set the key value in.
 * @return True if the value was set, otherwise false.
 */

bool setAppToken(const std::string &appID, const std::string &key, const std::string &value, const std::string &scope);

/**
 * @brief Delete an app token from the persistent store for a specific app.
 *
 * This function removes the value of a specific token from the persistent store for a specific app.
 *
 * @param appID The ID of the app.
 * @param key The key to remove the value for.
 * @param scope The scope to remove the key value from.
 * @return True if the value was removed, otherwise false.
 */
bool deleteAppToken(const std::string &appID, const std::string &key, const std::string &scope);

} // namespace Plugin
} // namespace WPEFramework