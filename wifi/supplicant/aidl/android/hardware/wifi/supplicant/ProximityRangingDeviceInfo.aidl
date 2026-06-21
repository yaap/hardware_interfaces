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

package android.hardware.wifi.supplicant;

import android.hardware.wifi.supplicant.ProximityRangingProtocolInfo;

/**
 * Defines device-level capabilities for Proximity Ranging, such as session
 * limits, concurrent role support and protocol-specific capabilities.
 */
@VintfStability
parcelable ProximityRangingDeviceInfo {
    /**
     * Maximum number of simultaneous continuous ranging sessions the
     * device can handle (Act as a seeker/initiator).
     */
    int maxNumContinuousRangingSeekerSessions;

    /**
     * Maximum number of simultaneous continuous ranging sessions the
     * device can handle (act as an advertiser/responder).
     */
    int maxNumContinuousRangingAdvertiserSessions;

    /**
     * Whether the device can support ranging initiator (seeker) and responder (advertiser)
     * role operation concurrently.
     */
    boolean isConcurrentIStaRStaOperationSupported;

    /**
     * Minimum allowed ranging interval supported by firmware in EDCA-based ranging, in ms.
     */
    int minAllowedRangingIntervalEdcaMs;

    /**
     * Minimum allowed ranging interval supported by firmware in Non-Trigger-Based (NTB)
     * ranging, in ms.
     */
    int minAllowedRangingIntervalNtbMs;

    /**
     * Protocol-specific capabilities/information for Proximity Ranging.
     */
    @nullable ProximityRangingProtocolInfo protocolInfo;

    /**
     * Whether the device supports MAC address randomization for proximity ranging
     * while the device is in a connected Wi-Fi (STA) state.
     */
    boolean isConnectedMacRandomizationSupported;
}
