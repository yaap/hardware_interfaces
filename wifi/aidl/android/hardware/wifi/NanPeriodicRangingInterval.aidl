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

package android.hardware.wifi;

/**
 * NAN Periodic Ranging Interval in Time Units.
 *
 * For more information, see the Wi-Fi Aware Spec 4.0, Table 97, Bits 3-5. From the 802.11 spec,
 * one Time Unit (TU) is equal to 1024 microseconds (approx. 1 ms).
 */
@VintfStability
@Backing(type="int")
enum NanPeriodicRangingInterval {
    /**
     * 128 TU
     */
    INTERVAL_128TU = 1 << 0,
    /**
     * 256 TU
     */
    INTERVAL_256TU = 1 << 1,
    /**
     * 512 TU
     */
    INTERVAL_512TU = 1 << 2,
    /**
     * 1024 TU
     */
    INTERVAL_1024TU = 1 << 3,
    /**
     * 2048 TU
     */
    INTERVAL_2048TU = 1 << 4,
    /**
     * 4096 TU
     */
    INTERVAL_4096TU = 1 << 5,
    /**
     * 8192 TU
     */
    INTERVAL_8192TU = 1 << 6,
}
