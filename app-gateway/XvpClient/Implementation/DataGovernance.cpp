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

#include "DataGovernance.h"
#include "UtilsLogging.h"

#include <interfaces/IPrivacy.h>

#define PRIVACY_CALLSIGN      "org.rdk.Privacy"

// TODO: Consider creating separate DataGovernance plugin, since AnalyticsSiftBackend lib has simmilar logic
#ifndef CET_MAP
#define CET_MAP ""
#endif
#ifndef CET_DROP_ON_ALL_TAGS
#define CET_DROP_ON_ALL_TAGS "false" // Default value for drop on all tags
#endif
#ifndef CET_EVENT_TYPE
#define CET_EVENT_TYPE "Product_Analytics" // Default value for CET event type
#endif

namespace WPEFramework
{
    namespace Plugin
    {

        DataGovernance::DataGovernance(PluginHost::IShell *shell)
        {
            ASSERT(shell != nullptr);
            mShell = shell;
            mShell->AddRef();

            // Set CetMap
            string cetMap(CET_MAP);
            if (!cetMap.empty())
            {
                JsonObject cetMapObject;
                cetMapObject.FromString(cetMap);
                auto cetMapIter = cetMapObject.Variants();

                while (cetMapIter.Next())
                {
                    if (cetMapIter.Current().Content() == WPEFramework::Core::JSON::Variant::type::STRING)
                    {
                        mCetMap[cetMapIter.Label()] = cetMapIter.Current().String();
                        LOGINFO("CetMap: \"%s\" : \"%s\"", cetMapIter.Label(), mCetMap[cetMapIter.Label()].c_str());
                    }
                }
            }
            else
            {
                // build default one
                LOGINFO("CetMap is default");
                mCetMap["xcal:personalization"] = "dataPlatform:cet:xvp:personalization:recommendation";
                mCetMap["xcal:productAnalytics"] = "dataPlatform:cet:xvp:analytics";
                mCetMap["xcal:continueWatching"] = "dataPlatform:cet:xvp:personalization:continueWatching";
                mCetMap["xcal:watchHistory"] = "dataPlatform:cet:xvp:personalization:continueWatching";
                mCetMap["xcal:businessAnalytics"] = "dataPlatform:cet:xvp:analytics:business";
            }
            mCetDropOnAllTags = (CET_DROP_ON_ALL_TAGS == "true");
            LOGINFO("Cet DropOnAllTags: %s", mCetDropOnAllTags ? "true" : "false");
            LOGINFO("Cet EventType: \"%s\"", CET_EVENT_TYPE);

        }

        DataGovernance::~DataGovernance()
        {
            if (mShell)
            {
                mShell->Release();
                mShell = nullptr;
            }
        }

        uint32_t DataGovernance::ResolveTags(const std::string &appId, DataGovernance::Tags &tags, bool &drop)
        {
             uint32_t result = Core::ERROR_NONE;
            drop = false;
            tags.clear();

            if (mCetMap.empty())
            {
                return result;
            }

            DataGovernance::PrivacySettings privacySettings;
            DataGovernance::PrivacyExclusionPolicies privacyExclusionPolicies;

            // Get Privacy Settings
            result = GetPrivacySettings(privacySettings);
            if (result == Core::ERROR_NONE)
            {
                result = GetPrivacyExclusionPolicies(privacyExclusionPolicies);
            }

            if (result == Core::ERROR_NONE)
            {
                drop = mCetDropOnAllTags;
                for (auto &tag : mCetMap)
                {
                    bool excluded = false;
                    bool propagationState = true;
                    if (!appId.empty() && privacyExclusionPolicies.find(tag.first) != privacyExclusionPolicies.end())
                    {
                        auto &exclusionPolicy = privacyExclusionPolicies.at(tag.first);
                        if (exclusionPolicy.dataEvents.find(CET_EVENT_TYPE) != exclusionPolicy.dataEvents.end())
                        {
                            // Go over entityReference to find first of appId
                            for (const auto &entity : exclusionPolicy.entityReference)
                            {
                                if (entity.find(appId) != std::string::npos)
                                {
                                    excluded = true;
                                    break;
                                }
                            }

                            propagationState = exclusionPolicy.derivativePropagation;
                        }
                    }

                    // Excluded by Exclusion Policy
                    if (excluded)
                    {
                        LOGINFO("Cet Tag %s added by Exclusion Policy", tag.second.c_str());
                        tags.insert(DataGovernance::Tag(tag.second, propagationState));
                    }
                    else
                    {
                        auto it = privacySettings.find(tag.first);
                        // Excluded by Privacy Settings: setting exists and equal not allowed
                        if (it != privacySettings.end())
                        {
                            if (it->second == false)
                            {
                                LOGINFO("Cet Tag %s added by Privacy Settings", tag.second.c_str());
                                tags.insert(DataGovernance::Tag(tag.second, true));
                            }
                            else // Not excluded. Drop only on all tags excluded
                            {
                                drop = false;
                            }
                        }
                    }
                }
            }

            return result;
        } 

        uint32_t DataGovernance::GetPrivacyExclusionPolicies(DataGovernance::PrivacyExclusionPolicies &policies)
        {
            policies.clear();
            uint32_t result = Core::ERROR_GENERAL;

            auto privacy = mShell->QueryInterfaceByCallsign<Exchange::IPrivacy>(PRIVACY_CALLSIGN);
            if (privacy != nullptr)
            {
                Exchange::IPrivacy::IStringIterator *polices = nullptr;
                result = privacy->GetExclusionPolicies(polices);
                if (result == Core::ERROR_NONE)
                {
                    if (polices != nullptr)
                    {
                        std::string policyName;
                        while (polices->Next(policyName))
                        {
                            Exchange::IPrivacy::IStringIterator *dataEvents = nullptr;
                            Exchange::IPrivacy::IStringIterator *entityReference = nullptr;
                            bool derivativePropagation = false;
                            uint32_t resultData = privacy->GetDataForExclusionPolicy(policyName, dataEvents, entityReference, derivativePropagation);
                            if (resultData != Core::ERROR_NONE)
                            {
                                LOGERR("Failed to get ExclusionPolicy %s", policyName.c_str());
                                continue;
                            }
                            PrivacyExclusionPolicy policy;
                            policy.derivativePropagation = derivativePropagation;
                            std::string dataEvent;
                            while (dataEvents && dataEvents->Next(dataEvent))
                            {
                                policy.dataEvents.insert(dataEvent);
                            }

                            std::string entityRef;
                            while (entityReference && entityReference->Next(entityRef))
                            {
                                policy.entityReference.insert(entityRef);
                            }

                            policies[policyName] = std::move(policy);

                            if (dataEvents)
                            {
                                dataEvents->Release();
                            }

                            if (entityReference)
                            {
                                entityReference->Release();
                            }
                            LOGINFO("Got ExclusionPolicy %s", policyName.c_str());
                        }
                        polices->Release();
                    }
                }
                else
                {
                    LOGERR("Failed to get ExclusionPolicies");
                }
                privacy->Release();
            }
            else
            {
                LOGERR("Failed to get Privacy interface");
            }

            return result;
        }

        uint32_t DataGovernance::GetPrivacySettings(DataGovernance::PrivacySettings &settings)
        {
            uint32_t result = Core::ERROR_GENERAL;
            settings.clear();
            auto privacy = mShell->QueryInterfaceByCallsign<Exchange::IPrivacy>(PRIVACY_CALLSIGN);

            if (privacy)
            {
                Exchange::IPrivacy::IPrivacySettingInfoIterator *allSettings = nullptr;
                if ((result = privacy->GetAllPrivacySettings(allSettings)) == Core::ERROR_NONE)
                {
                    if (allSettings != nullptr)
                    {
                        Exchange::IPrivacy::PrivacySettingInfo info;
                        while (allSettings->Next(info))
                        {
                            settings[info.settingName] = info.allowed;
                        }
                        allSettings->Release();
                    }
                    else
                    {
                        LOGERR("No Privacy settings");
                        result = Core::ERROR_UNAVAILABLE;
                    }
                }
                privacy->Release();
            }
            else
            {
                LOGERR("Failed to get Privacy interface");
            }

            return result;
        }

    }
}