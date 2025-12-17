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

package android.hardware.motioncontext;

import android.hardware.motioncontext.MotionState;

@VintfStability
parcelable MotionSubscription {
    // The MotionState of interest for this subscription. A single MotionEvent for this subscription
    // will be reported to the client per time the device state when the dwellTimeMs threshold is
    // satisfied.
    MotionState targetState = MotionState.UNKNOWN;

    // The minimum time in milliseconds the device must be continuously in the target state before
    // the subscription is considered satisfied. Exiting and reentering the target state will reset
    // this duration.
    //
    // If the minimum target detection latency is greater than this value, an event will be sent on
    // state confirmation. If this value is greater than the minimum target detection latency, the
    // effective dwellTime will be offset by the minimum target detection latency.
    //
    // For example: for a targetState of LOCATION_MOTION with a minimum target detection latency of
    // 10s and a dwellTimeMs of 5s, the effective dwellTime (event trigger) will be 10s.
    // In the same case, if a dwellTimeMs of 20s were specified, the effective dwellTime and event
    // delivery would be after 20s.

    int dwellTimeMs;
}
