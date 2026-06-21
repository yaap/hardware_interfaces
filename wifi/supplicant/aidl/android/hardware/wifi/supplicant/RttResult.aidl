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

import android.hardware.wifi.common.OuiKeyedData;
import android.hardware.wifi.supplicant.RttBw;
import android.hardware.wifi.supplicant.RttType;
import android.hardware.wifi.supplicant.WifiInformationElement;

/**
 * RTT results.
 */
@VintfStability
parcelable RttResult {
    /**
     * Ranging status.
     */
    @VintfStability
    @Backing(type="int")
    enum RttStatus {
        /**
         * General failure status.
         */
        SUCCESS = 0,
        /**
         * General failure status.
         */
        FAILURE = 1,
    }

    /**
     * Peer device mac address.
     */
    byte[6] addr;

    /**
     * Burst number in a multi-burst request.
     */
    int burstNum;

    /**
     * Total RTT measurement frames attempted.
     */
    int measurementNumber;

    /**
     * Total successful RTT measurement frames.
     */
    int successNumber;

    /**
     * Maximum number of "FTM frames per burst" supported by
     * the responder STA. Applies to 2-sided IEEE 802.11mc RTT only.
     * If responder overrides with larger value:
     * - for single-burst request, initiator will truncate the
     * larger value and send a TMR_STOP after receiving as
     * many frames as originally requested.
     * - for multi-burst request, initiator will return
     * failure right away.
     */
    byte numberPerBurstPeer;

    /**
     * Ranging status.
     */
    RttStatus status;

    /**
     * RTT type.
     */
    RttType type;

    /**
     * Average rssi value in dBM.
     */
    int rssi;

    /**
     * Rssi spread in dBM - optional.
     */
    int rssiSpread = -1;

    /**
     * Round trip time in picoseconds.
     */
    long rttPs;

    /**
     * Rtt standard deviation in picoseconds - optional.
     */
    long rttSdPs = -1;

    /**
     * Difference between max and min rtt times recorded in picoseconds - optional.
     */
    long rttSpreadPs = -1;

    /**
     * Distance in mm.
     */
    int distanceMm;

    /**
     * Standard deviation in mm.
     */
    int distanceSdMm;

    /**
     * Difference between max and min distance recorded in mm (optional).
     *
     * Note: Only applicable for IEEE 802.11mc
     */
    int distanceSpreadMm = -1;

    /**
     * Time of the measurement (in microseconds since boot).
     */
    long timestampUs;

    /**
     * Actual time taken by the FW to finish one burst measurement (in ms).
     */
    int burstDurationMs;

    /**
     * Number of bursts allowed by the responder.
     * Applies to 2-sided IEEE 802.11mc RTT only.
     */
    int negotiatedBurstNum;

    /**
     * For IEEE 802.11mc and IEEE 802.11az only.
     */
    WifiInformationElement lci;

    /**
     * For IEEE 802.11mc and IEEE 802.11az only.
     */
    WifiInformationElement lcr;

    /**
     * RTT channel frequency in MHz
     * If frequency is unknown, this will be set to 0.
     */
    int channelFrequencyMHz;

    /**
     * RTT packet bandwidth.
     * This value is an average bandwidth of the bandwidths of measurement frames. Cap the average
     * close to a specific valid RttBw.
     */
    RttBw packetBw;

    /**
     * Multiple transmissions of HE-LTF symbols in an HE (I2R) Ranging NDP. An HE-LTF repetition
     * value of 1 indicates no repetitions.
     *
     * Note: A required field for IEEE 802.11az result.
     */
    byte i2rTxLtfRepetitionCount;

    /**
     * Multiple transmissions of HE-LTF symbols in an HE (R2I) Ranging NDP. An HE-LTF repetition
     * value of 1 indicates no repetitions.
     *
     * Note: A required field for IEEE 802.11az result.
     */
    byte r2iTxLtfRepetitionCount;

    /**
     * Minimum non-trigger based (non-TB) dynamic measurement time in units of 100 microseconds
     * assigned by the IEEE 802.11az responder.
     *
     * After initial non-TB negotiation, if the next ranging request for this peer comes in between
     * [ntbMinMeasurementTime, ntbMaxMeasurementTime], vendor software shall do the NDPA sounding
     * sequence for dynamic non-TB measurement.
     *
     * If the ranging request for this peer comes sooner than minimum measurement time, vendor
     * software shall return the cached result of the last measurement including the time stamp
     * |RttResult.timestamp|.
     *
     * Reference: IEEE Std 802.11az-2022 spec, section 9.4.2.298 Ranging Parameters element.
     *
     * Note: A required field for IEEE 802.11az result.
     */
    long ntbMinMeasurementTimeIn100Us;

    /**
     * Maximum non-trigger based (non-TB) dynamic measurement time in units of 10 milliseconds
     * assigned by the IEEE 802.11az responder.
     *
     * After initial non-TB negotiation, if the next ranging request for this peer comes in between
     * [ntbMinMeasurementTime, ntbMaxMeasurementTime], vendor software shall do the NDPA sounding
     * sequence for dynamic non-TB measurement.
     *
     * If the ranging request for this peer comes later than the maximum measurement time, vendor
     * software shall clean up any existing IEEE 802.11ax non-TB ranging session and re-do the
     * non-TB ranging negotiation.
     *
     * Reference: IEEE Std 802.11az-2022 spec, section 9.4.2.298 Ranging Parameters element.
     *
     * Note: A required field for IEEE 802.11az result.
     */
    long ntbMaxMeasurementTimeIn10Ms;

    /**
     * Number of transmit space-time streams used. Value is in the range 1 to 8.
     *
     * Note: Maximum limit is ultimately defined by the number of antennas that can be supported.
     * A required field for IEEE 802.11az result.
     */
    byte numTxSpatialStreams;

    /**
     * Number of receive space-time streams used. Value is in the range 1 to 8.
     *
     * Note: Maximum limit is ultimately defined by the number of antennas that can be supported.
     * A required field for IEEE 802.11az result.
     */
    byte numRxSpatialStreams;

    /**
     * Optional vendor-specific parameters. Null value indicates
     * that no vendor data is provided.
     */
    @nullable OuiKeyedData[] vendorData;

    /**
     * Whether Secure HE-LTF is enabled.
     */
    boolean isSecureLtfEnabled;

    /**
     * Base Authentication and Key Management (AKM) protocol used for PASN. Represented as
     * at bitmap of |KeyMgmtMask|.
     */
    long baseAkm;

    /**
     * Pairwise cipher suite used for the PTKSA (Pairwise Transient Key Security Association).
     * Represented as a bitmap of |PairwiseCipherMask|.
     */
    long cipherSuite;

    /**
     * Secure HE-LTF protocol version used.
     */
    int secureHeLtfProtocolVersion;

    /**
     * Nominal duration between adjacent Availability Windows in
     * units of milli seconds.
     */
    int nominalTimeMs;

    /**
     * Duration of Availability Windows in
     * units of milli seconds.
     */
    int availabilityWindowTimeMs;

    /**
     * The number of measurements per Availability Window.
     */
    int measurementNumberPerAvailabilityWindow;

    /**
     * The number of range repetitions carried out for
     * distance calculation in NTB ranging.
     * Note: Only applicable for IEEE 802.11az result.
     */
    byte numNtbRepetitionsPerMeasurement;

    /**
     * Whether the device delayed sending the Location Measurement Report (LMR),
     * as defined in the IEEE 802.11az standard.
     */
    boolean isDelayedLmrEnabled;
}
