/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.gnss.gnss_assistance;

/**
 * Contains ephemeris parameters specific to Glonass satellites.
 * This is defined in RINEX 3.05 APPENDIX 10 and Glonass ICD v5.1, section 4.4.
 *
 * @hide
 */
@VintfStability
parcelable GlonassSatelliteEphemeris {
    /** Contains the set of parameters needed for Glonass satellite clock correction. */
    @VintfStability
    parcelable GlonassSatelliteClockModel {
        /**
         * Time of the clock in seconds (UTC).
         *
         * Represents the 'Epoch' field within the 'SV/EPOCH/SV CLK' record of GNSS
         * navigation message file in RINEX 3.05 Table A10.
         */
        long timeOfClockSeconds;

        /** Clock bias in seconds (-TauN). */
        double clockBias;

        /** Frequency bias (+GammaN). */
        double frequencyBias;

        /**
         * Frequency channel number.
         *
         * This is defined in Glonass ICD v5.1 section 3.3.1.1.
         */
        int frequencyChannelNumber;

        /**
         * L1/L2 group delay difference in seconds (DeltaTau).
         *
         * It is set to 0.999999999999E+09 if the value is not available.
         */
        double groupDelayDiffSeconds;
    }

    /** Contains Glonass orbit model parameters in PZ-90 coordinate system. */
    @VintfStability
    parcelable GlonassSatelliteOrbitModel {
        /** X position in kilometers. */
        double x;

        /** X velocity in kilometers per second. */
        double xDot;

        /** X acceleration in kilometers per second squared. */
        double xAccel;

        /** Y position in kilometers. */
        double y;

        /** Y velocity in kilometers per second. */
        double yDot;

        /** Y acceleration in kilometers per second squared. */
        double yAccel;

        /** Z position in kilometers. */
        double z;

        /** Z velocity in kilometers per second. */
        double zDot;

        /** Z acceleration in kilometers per second squared. */
        double zAccel;
    }

    /** Glonass health status healthy. */
    const int GLONASS_HEALTH_STATUS_HEALTHY = 0;

    /** Glonass health status unhealthy. */
    const int GLONASS_HEALTH_STATUS_UNHEALTHY = 1;

    /** Slot number. */
    int slotNumber;

    /** Satellite health which is set with the GLONASS_HEALTH_STATUS_* constants */
    int svHealth;

    /** Message frame time in seconds of the UTC week (tk+nd*86400). */
    double frameTimeSeconds;

    /** Age of current information in days (E). */
    int ageInDays;

    /** Update and validity interval in minutes (P1) **/
    int updateIntervalMinutes;

    /** Flag to indicate oddness(1) or evenness(0) of update interval (P2). */
    boolean isOddUpdateInterval;

    /** Flag to indicates if the satellite is a Glonass-M satellitee (M). */
    boolean isGlonassM;

    /** Satellite clock model. */
    GlonassSatelliteClockModel satelliteClockModel;

    /** Satellite orbit model. */
    GlonassSatelliteOrbitModel satelliteOrbitModel;
}
