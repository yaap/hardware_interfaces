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

import android.hardware.wifi.supplicant.DeviceIdentityKey;

/**
 * Pre-Association Security Negotiation (PASN) configuration.
 */
@VintfStability
parcelable PasnConfig {
    /**
     * Min length of SAE password.
     */
    const int PASN_SAE_PASSWORD_MIN_LEN_IN_BYTES = 1;

    /**
     * Max length of SAE password.
     */
    const int PASN_SAE_PASSWORD_MAX_LEN_IN_BYTES = 128;

    /**
     * Length of Pairwise Master Key (PMK).
     */
    const int PASN_SAE_PMK_LEN_IN_BYTES = 32;

    /**
     * Base Authentication and Key Management (AKM) protocol used for PASN. Represented as
     * a bitmap of |KeyMgmtMask|.
     */
    int baseAkm;

    /**
     * Pairwise cipher suite used for the PTKSA (Pairwise Transient Key Security Association).
     * Represented as a bitmap of |PairwiseCipherMask|.
     */
    int cipherSuite;

    /**
     * Passphrase for the base AKM. This can be null based on the AKM type.
     * Must be between |PASN_SAE_PASSWORD_MIN_LEN_IN_BYTES| and
     * |PASN_SAE_PASSWORD_MAX_LEN_IN_BYTES| in length.
     */
    @nullable byte[] passphrase;

    /**
     * Pairwise Master Key (PMK) for authenticated PASN mode.
     * Must be |PASN_SAE_PMK_LEN_IN_BYTES| in length.
     */
    @nullable byte[PASN_SAE_PMK_LEN_IN_BYTES] pmk;

    /**
     * The Ranging Seeker's device identity key (devIK) required for authenticated PASN mode in
     * proximity ranging. This key is always the Seeker's DevIK, regardless of the device's role.
     * The device's role as SEEKER or ADVERTISER is specified in
     * |ProximityRangingConfig.rangingServiceRole|.
     *
     * When this device is the SEEKER, it uses this key (its own DevIK) to generate a DIRA
     * attribute and include it in the PASN M1 frame.
     * When this device is the ADVERTISER, on receiving the PASN M1 frame, it uses this key
     * (the Seeker's DevIK, provisioned out-of-band) to match the Seeker's DIRA Tag and identify
     * the corresponding PMK/Password.
     */
    @nullable DeviceIdentityKey devIk;
}
