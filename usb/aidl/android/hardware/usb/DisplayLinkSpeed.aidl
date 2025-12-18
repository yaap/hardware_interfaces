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

package android.hardware.usb;

@VintfStability
@Backing(type="int")
/**
 * The speed of the Display link (DisplayPort alternate mode).
 */
enum DisplayLinkSpeed {
    /**
     * The speed of the Display link is unknown.
     */
    UNKNOWN = -1,
    /**
     * Other display link speed.
     *
     * The caller should refer to `getInterfaceVersion()` to determine the
     * meaning of OTHER. OTHER is a display link speed not defined in the
     * current HAL version.
     */
    OTHER = 0,
    /**
     * Reduced Bit rate (1.62 Gbps/lane)
     */
    RBR_1P62 = 1,
    /**
     * High Bit rate (2.7 Gbps/lane)
     */
    HBR_2P7 = 2,
    /**
     * High Bit rate 2 (5.4 Gbps/lane)
     */
    HBR2_5P4 = 3,
    /**
     * High Bit rate 3 (8.1 Gbps/lane)
     */
    HBR3_8P1 = 4,
    /**
     * Ultra High Bit rate 10 (10 Gbps/lane)
     */
    UHBR_10 = 5,
    /**
     * Ultra High Bit rate 13.5 (13.5 Gbps/lane)
     */
    UHBR_13P5 = 6,
    /**
     * Ultra High Bit rate 20 (20 Gbps/lane)
     */
    UHBR_20 = 7,
}
