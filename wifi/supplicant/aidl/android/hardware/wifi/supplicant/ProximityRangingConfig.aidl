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

/**
 * Proximity Ranging configuration
 */
@VintfStability
parcelable ProximityRangingConfig {
    /**
     * Ranging service role.
     */
    @VintfStability
    @Backing(type="int")
    enum RangingServiceRole {
        /**
         * Unknown role.
         */
        UNKNOWN = 0,
        /**
         * Seeker role.
         */
        SEEKER = 1,
        /**
         * Advertiser role.
         */
        ADVERTISER = 2,
    }

    /**
     * Controls the criteria for reporting the continuous range result.
     */
    @VintfStability
    @Backing(type="int")
    enum ProximityRangingIndication {
        CONTINUOUS_INDICATION_MASK = 1 << 0,
        INGRESS_MET_MASK = 1 << 1,
        EGRESS_MET_MASK = 1 << 2,
    }

    /**
     * Ranging measurement role.
     */
    @VintfStability
    @Backing(type="byte")
    enum RangingMeasurementRole {
        /**
         * Unknown role.
         */
        UNKNOWN = 0,
        /**
         * Ranging initiating station (ISTA) which initiates the range request.
         */
        INITIATOR_STA = 1,
        /**
         * Ranging responding station (RSTA) which receives the range request and starts the
         * measurement exchange.
         */
        RESPONDER_STA = 2,
    }

    /**
     * The ranging service role |RangingServiceRole|
     */
    RangingServiceRole rangingServiceRole;

    /*
     * Channel frequency in MHz on which the negotiation for
     * Security Association, Ranging Channel and FTM STAs roles is
     * conducted. This may be zero if the device discovery is conducted
     * over USD.
     */
    int discoveryChannelFrequencyMhz;

    /*
     * Sets the preferred ranging channel frequency in MHz for
     * measurement exchange. The supplicant may use this information
     * to derive the ranging channel.
     */
    int preferredRangingChannelFrequencyMhz;

    /**
     * Sets the preferred ranging measurement role |RangingMeasurementRole|.
     * Default role - RangingServiceRole.SEEKER takes the initiating station (ISTA) role and
     * RangingServiceRole.ADVERTISER takes the responding station (RSTA) role.
     */
    RangingMeasurementRole preferredRangingMeasurementRole;

    /**
     * Ranging Interval (in milliseconds) for conducting the range
     * measurement.
     * The supplicant uses this value as a hint for the desired
     * ranging interval, but the actual value will be negotiated with
     * the peer device and may be adjusted to ensure system stability.
     */
    int continuousRangingIntervalMillis;

    /**
     * Sets the desired ranging session time (in milliseconds) for a
     * continuous Ranging session. The supplicant uses this value and
     * |continuousRangingIntervalMillis| to
     * calculate the number of bursts required.
     */
    int continuousRangingSessionTimeMillis;

    /**
     * This field is only applicable when the |rangingServiceRole| is
     * |ADVERTISER|. When true, the advertiser requires a range report
     * after each measurement.
     */
    boolean advertiserRequiresRangeReport;

    /**
     * Bitmap of |ProximityRangingIndication| values indicating the type of ranging
     * feedback to be provided for a continuous proximity ranging session.
     */
    int configRangingIndications;

    /**
     * The ingress and egress distance in cm.
     * These values are used for proximity ranging when the corresponding
     * |ProximityRangingIndication.INGRESS_MET_MASK| and/or
     * |ProximityRangingIndication.EGRESS_MET_MASK| flags are set in |configRangingIndications|.
     */
    int distanceIngressCm;
    int distanceEgressCm;
}
