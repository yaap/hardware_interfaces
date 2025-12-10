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

package android.hardware.security.timestamp;

import android.hardware.security.timestamp.TimeStampReq;

/**
 * Interface for the TimeStamper HAL.
 */
@VintfStability
interface ITimeStamper {
    /**
     * Creates an RFC3161 timestamp token for the given timestamp request.
     *
     * The implementation should validate the incoming timestampReq and
     * generate a TimeStampToken as defined in RFC3161, Section 2.4.2. As
     * explained in Section 2.4.2, errors are communicated via the status
     * information contained in the output structure.
     *
     * @param timestampReq The RFC3161 timestamp request, containing the DER
     *        encoded TimeStampReq structure.
     * @return The RFC3161 TimeStampToken as a DER encoded byte array.
     *         Returns an empty array on unrecoverable failure.
     */
    byte[] createRfc3161TimestampToken(in TimeStampReq timestampReq);
}
