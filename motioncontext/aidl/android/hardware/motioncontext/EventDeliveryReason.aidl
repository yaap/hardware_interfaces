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

@VintfStability
@Backing(type="byte")
enum EventDeliveryReason {
    // The event was delivered because a MotionSubscription was triggered.
    MOTION_SUBSCRIPTION_TRIGGERED = 0,
    // The event was delivered because the client called configureMotionSubscription().
    MOTION_SUBSCRIPTION_CONFIGURED = 1,
}
