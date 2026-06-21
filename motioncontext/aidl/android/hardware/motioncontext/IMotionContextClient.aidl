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

import android.hardware.motioncontext.MotionSubscription;

@VintfStability
oneway interface IMotionContextClient {
    /**
     * API for the client to register motion subscriptions. When subscribed,
     * the client will receive MotionEvents that fit the criteria detailed by the subscriptions.
     *
     * Subscriptions exist until the next time the client calls configureMotionSubscription(),
     * persisting between any number of state transitions. Each subscription will generate at
     * most one MotionEvent per visit to a given state excepting the initial event generated
     * when this function is called
     *
     * The dwellTime of a subscription always references the time since the current state was
     * entered, not the time of configureMotionSubscription() being called. For cases where a
     * new subscription is added targeting a state that the device is already in, a MotionEvent
     * may or may not be generated depending on the value of the dwellTime and the time since
     * the device entered the current state:
     *  - If the device has been in the targeted state for less than the given dwellTime, a
     *     MotionEvent will be generated when the dwellTime is met.
     *  - If the device has already been in the current state longer than the given dwellTime, no
     *    new MotionEvent will be generated during this visit to the state.
     *
     * Any call to this function will trigger a callback providing the client with a MotionEvent
     * describing the current state. These events will have an EventDeliveryReason of
     * MOTION_SUBSCRIPTION_CONFIGURED. Clients may use this information to handle cases where
     * the subscription dwellTime has already been met since no triggered event will be
     * generated.
     *
     * When called, any existing subscriptions for this client will be removed.
     * An empty list will clear all subscriptions.
     *
     * If this registration call fails, the onMotionContextError() callback will be called with
     * an ErrorCode of MOTION_SUBSCRIPTION_FAILED. The client may retry later.
     *
     * @param subscriptions A list of all motion subscriptions to register.
     * @param cookie A cookie to be returned in any onMotionContextError() callbacks related to this
     *     configuration.
     */
    void configureMotionSubscription(in MotionSubscription[] subscriptions, in int cookie);

    /**
     * API for the client to acknowledge a MotionEvent. The client is expected to call this
     * function for every MotionEvent it receives in onMotionEvent(), after acquiring a wakelock
     * if necessary but before any long-running operation, using the sequenceNumber in the given
     * MotionEvent.
     *
     * Motion context change events are considered timing-sensitive, so to avoid indefinite
     * delays due to system suspend, the HAL must hold a wake lock until the ACK is received, or
     * a 1 second timeout expires, whichever comes first.
     *
     * @param sequenceNumber The sequence number of the MotionEvent to acknowledge.
     */
    void ackEvent(int sequenceNumber);
}
