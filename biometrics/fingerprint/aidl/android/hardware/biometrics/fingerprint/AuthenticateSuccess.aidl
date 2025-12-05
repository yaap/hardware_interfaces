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

package android.hardware.biometrics.fingerprint;

import android.hardware.keymaster.HardwareAuthToken;

/**
 * A successful result for a fingerprint authenticate operation.
 *
 * @hide
 */
@VintfStability
parcelable AuthenticateSuccess {
    /** Fingerprint that was accepted. */
    int enrollmentId;

    /**
     * If the sensor is configured as SensorStrength::STRONG, a non-null attestation that
     * a fingerprint was accepted. The HardwareAuthToken's "challenge" field must be set
     * with the operationId passed in during ISession#authenticate. If the sensor is NOT
     * SensorStrength::STRONG, the HardwareAuthToken MUST be null
     */
    HardwareAuthToken hat;

    /**
     * Optional vendor-defined metadata related to this successful authentication result.
     *
     * This framework will not use this metadata so it cannot be used to indicate a
     * partial, inconclusive, or anything other than a successful result. Vendors may use
     * this to provide synchronous feedback to users when this event occurs, much like
     * they may use the vendor codes in ISessionCallback#onAcquired(AcquiredInfo, int)
     * during the authentication process before a result is available.
     */
    ParcelableHolder metadata;
}
