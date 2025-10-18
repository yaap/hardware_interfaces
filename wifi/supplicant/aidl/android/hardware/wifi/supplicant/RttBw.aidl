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
 * RTT bandwidth.
 */
@VintfStability
enum RttBw {
    /** Invalid bandwidth. */
    INVALID = 0,
    /** 5 MHz bandwidth. */
    BW_5MHZ = 1 << 0,
    /** 10 MHz bandwidth. */
    BW_10MHZ = 1 << 1,
    /** 20 MHz bandwidth. */
    BW_20MHZ = 1 << 2,
    /** 40 MHz bandwidth. */
    BW_40MHZ = 1 << 3,
    /** 80 MHz bandwidth. */
    BW_80MHZ = 1 << 4,
    /** 160 MHz bandwidth. */
    BW_160MHZ = 1 << 5,
    /** 320 MHz bandwidth. */
    BW_320MHZ = 1 << 6,
}
