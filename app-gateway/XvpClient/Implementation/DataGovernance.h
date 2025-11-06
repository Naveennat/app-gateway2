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

#include "../Module.h"

#include <map>
#include <set>
#include <unordered_set>

namespace WPEFramework {
namespace Plugin {


    class DataGovernance {
    public:
        DataGovernance(PluginHost::IShell *shell);
        ~DataGovernance();

        struct Tag
        {
            string name;
            bool propagationState;

            Tag() : name(), propagationState(false) {}
            Tag(const string& n, bool p) : name(n), propagationState(p) {}
            Tag(const Tag& other) = default;
            Tag& operator=(const Tag& other) = default;

            bool operator==(const Tag& other) const {
                return name == other.name;
            }
        };

        struct TagHash
        {
            size_t operator()(const Tag &t) const noexcept
            {
                return std::hash<std::string>{}(t.name); // hash by name only
            }
        };

        struct TagEq
        {
            bool operator()(const Tag &a, const Tag &b) const noexcept
            {
                return a.name == b.name; // equality by name only
            }
        };

        using Tags = std::unordered_set<Tag, TagHash, TagEq>;

        uint32_t ResolveTags(const std::string &appId, Tags &tags, bool &drop);

    private:

        struct PrivacyExclusionPolicy
        {
            std::set<std::string> dataEvents;
            std::set<std::string> entityReference;
            bool derivativePropagation;

            PrivacyExclusionPolicy()
                : derivativePropagation(false)
            {
            }
        };

        // ExclusionPolicies as pairs of <RawPrivacySettingName, PrivacyExclusionPolicy struct>
        using PrivacyExclusionPolicies = std::map<string, PrivacyExclusionPolicy>;
        // PrivacySettings as pairs of <RawPrivacySettingName, allowed>
        using PrivacySettings = std::map<string, bool>;

        uint32_t GetPrivacyExclusionPolicies(PrivacyExclusionPolicies &policies);
        uint32_t GetPrivacySettings(PrivacySettings &settings);

        PluginHost::IShell *mShell;
        std::map<std::string, std::string> mCetMap;
        bool mCetDropOnAllTags;

    };
}
}