/**
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

package android.hardware.wifi.supplicant;

import android.hardware.wifi.supplicant.RttResult;

/**
 * RTT Response and Event Callbacks.
 */
@VintfStability
oneway interface ISupplicantWifiRttControllerEventCallback {
    /**
     * Status/Progress code for a continuous Proximity Ranging (PR) session.
     */
    @VintfStability
    @Backing(type="int")
    enum ContinuousRangingStatusCode {
        /** Unknown or invalid status. Used for forward compatibility. */
        UNKNOWN = 0,
        /** Proximity Ranging negotiation started. */
        PR_RANGE_NEGOTIATION_STARTED = 1,
        /** Proximity Ranging negotiation succeeded. */
        PR_RANGE_NEGOTIATION_SUCCEEDED = 2,
        /** Proximity Ranging started, with this device in the ISTA (initiating station) role. */
        PR_STARTED_RANGE_REQUESTS_ISTA_ROLE = 3,
        /** Proximity Ranging started, with this device in the RSTA (responding station) role. */
        PR_STARTED_RANGE_REQUESTS_RSTA_ROLE = 4,
    }

    /**
     * The reason for terminating the ranging session
     */
    @VintfStability
    @Backing(type="int")
    enum ContinuousRangingTerminateReasonCode {
        /** Unknown reason. */
        UNKNOWN = 0,
        /** Session terminated due to a timeout. */
        TIMEOUT = 1,
        /** Session terminated by a user request. */
        USER_REQUEST = 2,
        /** Session aborted due to a concurrency issue (e.g., another Wi-Fi operation). */
        ABORT_CONCURRENCY = 3,
        /** Session terminated upon receiving a termination request from the peer. */
        RECEIVED_RTT_TERMINATE = 4,
        /** Proximity Ranging (PR) negotiation failed. */
        PR_RANGE_NEG_FAILED = 5,
    }

    /**
     * Invoked when an RTT result is available.
     *
     * @param cmdId Command Id corresponding to the original request.
     * @param results Vector of |RttResult| instances.
     */
    void onResults(in int cmdId, in RttResult[] results);

    /**
     * Indicates a continuous ranging status or progress
     * @param cmdId Command Id corresponding to the original request.
     * @param code The status code |ContinuousRangingStatusCode|
     */
    void onContinuousRangingStatusChanged(in int cmdId, in ContinuousRangingStatusCode code);

    /**
     * Called when the continuous ranging session has been terminated.
     * This indicates that no further results will be delivered.
     *
     * @param cmdId Command Id corresponding to the original request.
     * @param reason The reason |RangingTerminateReasonCode| for
     * the session termination, such as explicit termination by the
     * app or due to a system event.
     */
    void onContinuousRangingTerminated(
            in int cmdId, in ContinuousRangingTerminateReasonCode reason);
}
