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

package android.hardware.contexthub;

/** Contains the requirements for the allocation of a new shared data region. */
@VintfStability
parcelable SharedDataRegionRequirements {
    /** The minimum size of the region in bytes. */
    long sizeBytes;

    /** The Android permissions required to access the shared data region. */
    String[] permissions;

    /** The other message hubs whose endpoints will need to access this region. */
    long[] targetHubIds;
}