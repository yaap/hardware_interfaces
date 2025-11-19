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
 * RTT preamble.
 */
@VintfStability
enum RttPreamble {
    /** Invalid preamble. */
    INVALID = 0,
    /** Legacy preamble. */
    LEGACY = 1 << 0,
    /** High-Throughput (HT) preamble. */
    HT = 1 << 1,
    /** Very-High-Throughput (VHT) preamble. */
    VHT = 1 << 2,
    /** High-Efficiency (HE) preamble. */
    HE = 1 << 3,
    /** Extremely-High-Throughput (EHT) preamble. */
    EHT = 1 << 4,
}
