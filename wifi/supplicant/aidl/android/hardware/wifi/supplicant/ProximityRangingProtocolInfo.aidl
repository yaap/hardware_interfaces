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

import android.hardware.wifi.supplicant.RttBw;
import android.hardware.wifi.supplicant.RttPreamble;

/**
 * Defines protocol-specific capabilities/information for Proximity Ranging.
 * This structure can represent the capabilities of the local device or a
 * peer device discovered over the air.
 */
@VintfStability
parcelable ProximityRangingProtocolInfo {
    /**
     * The device name is a friendly name of the Proximity Ranging device.
     */
    String deviceName;

    /**
     * Whether EDCA-based ranging is supported.
     */
    boolean isEdcaBasedRangingSupported;

    /**
     * Whether NTB (Non-Trigger-Based) ranging with a non-secure
     * Long Training Field (LTF) is supported.
     */
    boolean isNtbNonSecureLtfRangingSupported;

    /**
     * Whether NTB (Non-Trigger-Based) ranging with a secure
     * Long Training Field (LTF) is supported.
     */
    boolean isNtbSecureLtfRangingSupported;

    /**
     * Whether unauthenticated PASN mode is supported,
     * i.e., when there are no authentication credentials
     * (no Password and no PMK).
     */
    boolean isUnauthenticatedPasnModeSupported;

    /**
     * Whether authenticated PASN mode is supported,
     * i.e., when both devices share authentication credentials
     * (e.g., a Password or PMK along with the device identity key).
     */
    boolean isAuthenticatedPasnModeSupported;

    /**
     * Whether the Initiating Station (ISTA) role for
     * EDCA-based ranging is supported.
     */
    boolean isEdcaBasedIstaRoleSupported;

    /**
     * Whether the Responding Station (RSTA) role for
     * EDCA-based ranging is supported.
     */
    boolean isEdcaBasedRstaRoleSupported;

    /**
     * Whether the Initiating Station (ISTA) role for
     * NTB (Non-Trigger-Based) ranging is supported.
     */
    boolean isNtbIstaRoleSupported;

    /**
     * Whether the Responding Station (RSTA) role for
     * NTB (Non-Trigger-Based) ranging is supported.
     */
    boolean isNtbRstaRoleSupported;

    /**
     * The maximum supported packet bandwidth for
     * EDCA based ranging.
     */
    RttBw maxSupportedPacketBandwidthEdcaBased;

    /**
     * The maximum supported preamble or format for
     * EDCA based ranging.
     */
    RttPreamble maxSupportedPreambleEdcaBased;

    /**
     * The maximum supported packet bandwidth for
     * NTB ranging.
     */
    RttBw maxSupportedPacketBandwidthNtb;

    /**
     * The maximum supported preamble or format for
     * NTB ranging.
     */
    RttPreamble maxSupportedPreambleNtb;

    /**
     * Whether proximity ranging is supported on the 6GHz band.
     */
    boolean is6GHzSupported;

    /**
     * Maximum number of transmit antennas supported for ranging.
     */
    int maxNumTxAntennas;

    /**
     * Maximum number of receive antennas supported for ranging.
     */
    int maxNumRxAntennas;
}
