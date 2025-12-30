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

/**
 * Used to indicate the motion state of the device.
 */
@VintfStability
@Backing(type="byte")
enum MotionState {
    // Motion state is unspecified or unknown.
    UNKNOWN = 0,

    // Device has not seen meaningful accelerometer activity within the last 2s, and was not likely
    // to be experiencing Location-changing motion within the last 5s.
    STILL = 1,

    // Device has seen meaningful accelerometer activity within the last 2s, and was not likely to
    // be experiencing Location-changing motion within the last 5s.
    LOCAL_MOTION = 2,

    // Device is likely to be moving in a way that may produce a meaningful change in semantic
    // location, for example walking, running, or biking, or riding in a moving vehicle that is
    // sustained for long enough to be detected.
    // For the purposes of defining detection latency, the starting point is considered the time in
    // which the device started moving in a recognized pattern, not necessarily the point in which
    // the device has moved a significant amount.
    // Implementations should strive to confidently detect location changing motion within 15
    // seconds. Note that depending on the motion pattern and detection mechanism, achievable
    // latency may be higher, but nearly all scenarios must be detected within 5 minutes.
    LOCATION_CHANGING_MOTION = 3,
}
