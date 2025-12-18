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

import android.hardware.motioncontext.EventDeliveryReason;
import android.hardware.motioncontext.MotionState;

@VintfStability
parcelable MotionEvent {
    // The previous motion state of the device.
    MotionState previousState = MotionState.UNKNOWN;

    // The current motion state of the device.
    MotionState currentState = MotionState.UNKNOWN;

    // The duration in milliseconds since the device determined it is in the current state.
    // This number is never expected to be significantly lower than the minimum target detection
    // latency of the associated state. Similarly, it will never be lower than the dwellTimeMs of
    // any associated subscription.
    int durationMs;

    // The reason the event was delivered. Can be used by the client to distinguish between
    // MotionEvents triggered by various means.
    EventDeliveryReason deliveryReason = EventDeliveryReason.MOTION_SUBSCRIPTION_TRIGGERED;

    // An arbitratry, unique sequence number for the motion event. This should be used by the
    // IMotionContextClient in ackEvent() to acknowledge receipt of the event.
    int sequenceNumber;
}
