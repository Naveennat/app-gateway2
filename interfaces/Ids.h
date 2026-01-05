/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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
 */

#pragma once

// IMPORTANT:
// This repository carries an Ids.h primarily so plugin code can simply do:
//   #include <Ids.h>
//
// The Thunder/WPEFramework SDK already provides the canonical interface ID list at:
//   <WPEFramework/interfaces/Ids.h>
//
// If we define IDS here as well, the build will see *two* different headers defining
// the same enum WPEFramework::Exchange::IDS (one from the SDK, one from this repo),
// which results in compilation failures ("multiple definition of enum IDS").
//
// Therefore, this file is intentionally a thin wrapper that includes the SDK header.
// The build system adds the SDK include path, so this resolves via normal include paths.

#include <interfaces/Ids.h>
