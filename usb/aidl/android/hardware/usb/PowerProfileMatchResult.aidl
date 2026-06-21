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

package android.hardware.usb;

import android.hardware.usb.PowerProfile;

/**
 * Describes the match result between a port sink/source PowerProfile and the
 * corresponding partner source/sink PowerProfile.
 */
@VintfStability
parcelable PowerProfileMatchResult {
    /**
     * Index for a local port sink/source PowerProfile this structure corresponds to that
     * matches with the partner port source/sink PowerProfile in {@code partnerIndex}.
     */
    int portIndex = -1;

    /**
     * Index for a partner port source/sink PowerProfile this structure corresponds to that
     * matches with the local port sink/source PowerProfile in {@code portIndex}.
     */
    int partnerIndex = -1;

    /**
     * Stores the match result between the port and partner PowerProfiles. Standard types are
     * expected to report a PowerProfile of the same type, but a match that includes a vendor
     * PowerProfile can return a vendor PowerProfile or the type of the matching PowerProfile.
     */
    PowerProfile result;
}
