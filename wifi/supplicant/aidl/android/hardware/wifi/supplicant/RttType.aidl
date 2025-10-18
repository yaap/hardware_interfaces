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

/**
 * RTT Types.
 */
@VintfStability
@Backing(type="int")
enum RttType {
    /**
     * Unknown RTT type.
     */
    UNKNOWN = 0,

    /**
     * Two-sided RTT 11mc type.
     */
    TWO_SIDED_11MC = 1,

    /**
     * Two-sided RTT 11az non trigger based (non-TB) secure type.
     */
    TWO_SIDED_11AZ_NTB_SECURE = 2,
}
