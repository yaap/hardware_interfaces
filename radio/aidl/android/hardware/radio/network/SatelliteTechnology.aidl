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

package android.hardware.radio.network;

/**
 * SatelliteTechnology
 * Represents the different types of satellite technologies that a device can be connected to.
 * This enum helps the framework identify whether the device is on a terrestrial network or
 * a non-terrestrial (satellite) network, and which specific satellite technology is being used.
 * Moreover, this enum is also used by modem to prioritize the scanning and attaching process.
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum SatelliteTechnology {
    /** Default value for this which is set when it is not a satellite network. */
    SAT_TECH_NONE,
    /** This value indicates that the satellite deployment uses NB IOT NTN technology. */
    SAT_TECH_NB_IOT_NTN,
    /**
     * This value indicates that the device is connected via a Direct-to-Cell (DTC) technology
     * over a Non-Terrestrial Network (NTN). DTC enables standard mobile devices (e.g., smartphones)
     * to connect directly to satellites, which act as orbiting base stations. The specific
     * underlying radio access technology (RAT) depends on the context:
     * <ul>
     * <li>When reported within {@link EutranRegistrationInfo}, it indicates LTE-based DTC.
     *     This maps to {@code SatelliteManager.NT_RADIO_TECHNOLOGY_LTE_DTC}.</li>
     * <li>When reported within {@link NrRegistrationInfo}, it indicates 5G NR-based DTC.
     *     This maps to {@code SatelliteManager.NT_RADIO_TECHNOLOGY_NR_DTC}.</li>
     * </ul>
     */
    SAT_TECH_DTC,
    /**
     * This value indicates that the device is connected via a 3GPP-standardized
     * Non-Terrestrial Network (NTN) technology that is not specifically NB-IoT or DTC.
     * The exact technology depends on the context:
     * <ul>
     * <li>When reported within {@link NrRegistrationInfo}, it indicates 5G NR-based NTN.
     *     This covers the general 3GPP specifications for using NR in satellite environments
     *     (e.g., as defined in 3GPP Release 17 and later). This maps to
     *     {@code SatelliteManager.NT_RADIO_TECHNOLOGY_NR_NTN}.</li>
     * </ul>
     * <p>Modem should not report NTN with {@link EutranRegistrationInfo} since 3GPP does not
     * support LTE-based NTN.
     */
    SAT_TECH_3GPP_NTN
}
