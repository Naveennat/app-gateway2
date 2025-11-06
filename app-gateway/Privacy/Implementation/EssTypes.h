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

#include <string>
#include <map>
#include <set>
#include <list>

namespace WPEFramework
{
    namespace Plugin
    {
        enum EssExclusionPolicyType
        {
            EssExclusionPolicyTypeProductAnalytics,
            EssExclusionPolicyTypeBusinessAnalytics,
            EssExclusionPolicyTypePersonalization
        };

        struct EssConsentString
        {
            std::string gppValue;
            std::string gppSid;
            std::string decoded;
            uint32_t expireTime;
            EssConsentString()
                : gppValue("")
                , gppSid("")
                , decoded("")
                , expireTime(0)
            {
            }

            bool operator==(const EssConsentString& other) const
            {
                return gppValue == other.gppValue &&
                       gppSid == other.gppSid &&
                       decoded == other.decoded;
            }

            bool operator!=(const EssConsentString& other) const
            {
                return !(*this == other);
            }
        };

        struct EssPrivacySettingData
        {
            bool available;
            bool allowed;
            bool isDefault;
            std::string expiration;
            std::string updated;
            std::string ownerReference;
            std::string entityReference;

            EssPrivacySettingData()
                : available(false)
                , allowed(false)
                , isDefault(false)
                , expiration("")
                , updated("")
                , ownerReference("")
                , entityReference("")
            {
            }

            bool operator==(const EssPrivacySettingData& other) const
            {
                return available == other.available &&
                       allowed == other.allowed &&
                       isDefault == other.isDefault &&
                       expiration == other.expiration &&
                       updated == other.updated &&
                       ownerReference == other.ownerReference &&
                       entityReference == other.entityReference;
            }

            bool operator!=(const EssPrivacySettingData& other) const
            {
                return !(*this == other);
            }
        };

        struct EssPrivacySettings
        {
            EssPrivacySettingData continueWatching;
            EssPrivacySettingData personalization;
            EssPrivacySettingData productAnalytics;
            EssPrivacySettingData watchHistory;
            EssPrivacySettingData acrCollection;
            EssPrivacySettingData appContentAdTargeting;
            EssPrivacySettingData cameraAnalytics;
            EssPrivacySettingData primaryBrowseAdTargeting;
            EssPrivacySettingData primaryContentAdTargeting;
            EssPrivacySettingData remoteDiagnostics;
            EssPrivacySettingData unentitledPersonalization;
            EssPrivacySettingData unentitledContinueWatching;

            bool operator==(const EssPrivacySettings& other) const
            {
                return continueWatching == other.continueWatching &&
                       productAnalytics == other.productAnalytics &&
                       watchHistory == other.watchHistory &&
                       acrCollection == other.acrCollection &&
                       appContentAdTargeting == other.appContentAdTargeting &&
                       cameraAnalytics == other.cameraAnalytics &&
                       personalization == other.personalization &&
                       primaryBrowseAdTargeting == other.primaryBrowseAdTargeting &&
                       primaryContentAdTargeting == other.primaryContentAdTargeting &&
                       remoteDiagnostics == other.remoteDiagnostics &&
                       unentitledPersonalization == other.unentitledPersonalization &&
                       unentitledContinueWatching == other.unentitledContinueWatching;
            }

            bool operator!=(const EssPrivacySettings& other) const
            {
                return !(*this == other);
            }
        };

        struct EssExclusionPolicyData
        {
            std::set<std::string> dataEvents;
            std::set<std::string> entityReference;
            bool derivativePropagation;
            bool available;

            EssExclusionPolicyData()
                : derivativePropagation(false)
                , available(false)
            {
            }

            bool operator==(const EssExclusionPolicyData& other) const
            {
                return dataEvents == other.dataEvents &&
                       entityReference == other.entityReference &&
                       derivativePropagation == other.derivativePropagation &&
                       available == other.available;
            }

            bool operator!=(const EssExclusionPolicyData& other) const
            {
                return !(*this == other);
            }
        };

        struct EssExclusionPolicies
        {
            EssExclusionPolicyData productAnalytics;
            EssExclusionPolicyData businessAnalytics;
            EssExclusionPolicyData personalization;

            bool operator==(const EssExclusionPolicies& other) const
            {
                return productAnalytics == other.productAnalytics &&
                       businessAnalytics == other.businessAnalytics &&
                       personalization == other.personalization;
            }

            bool operator!=(const EssExclusionPolicies& other) const
            {
                return !(*this == other);
            }
        };

        #define EssPrivacySettingContinueWatching  "xcal:continueWatching"
        #define EssPrivacySettingPersonalization  "xcal:personalization"
        #define EssPrivacySettingProductAnalytics  "xcal:productAnalytics"
        #define EssPrivacySettingWatchHistory  "xcal:watchHistory"
        #define EssPrivacySettingAcrCollection           "xcal:acr"
        #define EssPrivacySettingAppContentAdTargeting  "xcal:appContentAdTargeting"
        #define EssPrivacySettingCameraAnalytics  "xcal:cameraAnalytics"
        #define EssPrivacySettingPrimaryBrowseAdTargeting  "xcal:primaryBrowseAdTargeting"
        #define EssPrivacySettingPrimaryContentAdTargeting  "xcal:primaryContentAdTargeting"
        #define EssPrivacySettingRemoteDiagnostics  "xcal:remoteDiagnostics"
        #define EssPrivacySettingUnentitledPersonalization  "xcal:unentitledPersonalization"
        #define EssPrivacySettingUnentitledContinueWatching  "xcal:unentitledContinueWatching"

        #define EssExclusionPolicyProductAnalytics  "xcal:productAnalytics"
        #define EssExclusionPolicyBusinessAnalytics  "xcal:businessAnalytics"
        #define EssExclusionPolicyPersonalization  "xcal:personalization"

        #define EssConsentStringPrimaryContentAdTargeting  "primaryContentAdTargeting"
        #define EssConsentStringPrimaryBrowseAdTargeting  "primaryBrowseAdTargeting"
        #define EssConsentStringAppContentAdTargeting  "appContentAdTargeting"
    } // Plugin
} // WPEFramework
