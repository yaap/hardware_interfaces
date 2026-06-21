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

import android.hardware.wifi.common.OuiKeyedData;
import android.hardware.wifi.supplicant.MacAddress;
import android.hardware.wifi.supplicant.ProximityRangingConfig;
import android.hardware.wifi.supplicant.RttBw;
import android.hardware.wifi.supplicant.RttPreamble;
import android.hardware.wifi.supplicant.RttSecureConfig;
import android.hardware.wifi.supplicant.RttType;

/**
 * RTT configuration.
 */
@VintfStability
parcelable RttConfig {
    /**
     * RTT peer types.
     */
    @VintfStability
    @Backing(type="int")
    enum RttPeerType {
        /**
         * Invalid peer type.
         */
        INVALID = 0,
        /**
         * Peer type STA for proximity ranging.
         */
        STA = 1,
    }

    /**
     * Peer device mac address.
     */
    byte[6] addr;
    /**
     * Supports only 2-sided RTT (IEEE 802.11mc or IEEE 802. 11az).
     */
    RttType type;
    /**
     * Optional peer device hint (STA, P2P, AP).
     * STA type for Proximity ranging
     */
    RttPeerType peer;

    /**
     * Number of frames per burst.
     * Minimum value = 1, Maximum value = 31
     * For IEEE 80211mc ranging, this equals the number of FTM frames
     * to be attempted in a single burst. This also
     * equals the number of FTM frames that the
     * initiator will request that the responder sends
     * in a single frame.
     *
     * Note: Applicable to IEEE 802.11mc only.
     */
    byte numFramesPerBurst;

    /**
     * The number of range repetitions set for
     * distance calculation in NTB ranging.
     * Minimum value = 1, Maximum value = 8
     *
     * Note: Applicable to IEEE 802.11az only.
     */
    byte numNtbRepetitionsPerMeasurement;

    /**
     *
     * Maximum number of retries that the initiator can
     * retry an FTMR frame.
     * Minimum value = 0, Maximum value = 3
     */
    byte numRetriesPerFtmr;

    /**
     * Whether to request location civic info.
     */
    boolean mustRequestLci;

    /**
     * Whether to request location civic records.
     */
    boolean mustRequestLcr;

    /**
     * Valid values will be 2-11 and 15 as specified by the IEEE 802.11mc std for
     * the FTM parameter burst duration. In a multi-burst
     * request, if responder overrides with larger value,
     * the initiator will return failure. In a single-burst
     * request, if responder overrides with larger value,
     * the initiator will send TMR_STOP to terminate RTT
     * at the end of the burst_duration it requested.
     * Refer IEEE802.11 specification Table 9-279 for burst duration encoding.
     *
     * Note: Applicable to IEEE 802.11mc only.
     */
    byte burstDuration;

    /**
     * RTT preamble to be used in the RTT frames.
     */
    RttPreamble preamble;

    /**
     * RTT BW to be used in the RTT frames.
     */
    RttBw bw;

    /**
     * IEEE 802.11az Non-Trigger-based (non-TB) minimum measurement time in units of 100
     * microseconds.
     *
     * Reference: IEEE Std 802.11az-2022 spec, section 9.4.2.298 Ranging Parameters element.
     */
    long ntbMinMeasurementTimeIn100Us;

    /**
     * IEEE 802.11az Non-Trigger-based (non-TB) maximum measurement time in units of 10
     * milliseconds.
     *
     * Reference: IEEE Std 802.11az-2022 spec, section 9.4.2.298 Ranging Parameters element.
     */
    long ntbMaxMeasurementTimeIn10Millis;

    /**
     * Optional vendor-specific parameters. Null value indicates
     * that no vendor data is provided.
     */
    @nullable OuiKeyedData[] vendorData;

    /**
     * Secure Ranging configuration
     */
    @nullable RttSecureConfig secureConfig;

    /**
     * Proximity Ranging configuration
     */
    @nullable ProximityRangingConfig pdConfig;
}
