/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <android-base/logging.h>

#ifdef WOULD_LOG
#undef WOULD_LOG

// WOULD_LOG is redefined to avoid costs of looking up whether the particular
// LOG_TAG is currently enabled.
#define WOULD_LOG(severity)                                                               \
    (UNLIKELY((SEVERITY_LAMBDA(severity)) >= ::android::base::GetMinimumLogSeverity()) || \
     MUST_LOG_MESSAGE(severity))

#endif  // WOULD_LOG
