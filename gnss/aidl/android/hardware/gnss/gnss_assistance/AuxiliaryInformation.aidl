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

import android.hardware.gnss.GnssSignalType;

/**
 * Contains parameters to provide additional information dependent on the GNSS constellation.
 *
 * @hide
 */
@VintfStability
parcelable AuxiliaryInformation {
    /**
     * BDS B1C Satellite orbit type.
     *
     * This is defined in BDS-SIS-ICD-B1I-3.0, section 3.1.
     */
    @VintfStability
    @Backing(type="int")
    enum BeidouB1CSatelliteOrbitType {
        UNDEFINED = 0,
        GEO = 1,
        IGSO = 2,
        MEO = 3
    }

    /**
     * Pseudo-random or satellite ID number for the satellite, a.k.a. Space Vehicle (SV), or
     * OSN number for Glonass. The distinction is made by looking at the constellation field.
     * Values must be in the range of:
     *
     * - GPS:    1-32
     * - Glonass: 1-25
     * - QZSS:    183-206
     * - Galileo: 1-36
     * - Beidou:  1-63
     */
    int svid;

    /** The list of available signal types for the satellite. */
    GnssSignalType[] availableSignalTypes;

    /**
     * Glonass carrier frequency number of the satellite. This is required for Glonass.
     *
     * This is defined in Glonass ICD v5.1 section 3.3.1.1.
     */
    int frequencyChannelNumber;

    /** BDS B1C satellite orbit type. This is required for Beidou. */
    BeidouB1CSatelliteOrbitType satType;
}
