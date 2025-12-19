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

/**
 * USB speed.
 *
 * The format of the variants is USB[Version]_[Speed]_[Unit] where:
 *  - [Version] is the major USB version where the speed was introduced
 *  - [Speed] is the speed.
 *  - [Unit] is the unit of the speed (megabits per second, gigabits per
 *    second, etc.)
 */
@VintfStability
@Backing(type="int")
enum UsbSpeed {
    /**
     * Unknown speed.
     */
    UNKNOWN = -1,
    /**
     * Other speed.
     *
     * The caller should refer to `getInterfaceVersion()` to determine the
     * meaning of OTHER. OTHER is a USB speed not defined in the current HAL
     * version.
     */
    OTHER = 0,
    /**
     * USB 1.0 Low Speed (1.5 Mbps).
     */
    USB1_1P5_MBITPS = 1,
    /**
     * USB 1.0/1.1 Full Speed (12 Mbps).
     */
    USB1_12_MBITPS = 2,
    /**
     * USB 2.0 High Speed (480 Mbps).
     */
    USB2_480_MBITPS = 3,
    /**
     * USB 3.2 Gen 1x1 SuperSpeed/3.1 Gen 1/USB 3.0 (5 Gbps).
     */
    USB3_5_GBITPS = 4,
    /**
     * USB 3.2 Gen 2x1 SuperSpeed+/USB 3.1 Gen 2 (10 Gbps).
     */
    USB3_10_GBITPS = 5,
    /**
     * USB 3.2 Gen 2x2 SuperSpeed+ (20 Gbps).
     *
     * This uses two lanes of Gen 2 (10 Gbps) signaling.
     */
    USB3_20_GBITPS = 6,
    /**
     * USB4 Gen 2 speed (20 Gbps).
     *
     * Refers to a USB4 link operating with Gen 2 signaling (10 Gbps)
     * bonded across two lanes.
     *
     * Note: This is distinct from USB 3.2 Gen 2x2 (USB3_20_GBITPS).
     * USB3_20_GBITPS refers to the capabilities of the USB3 Host controller,
     * which can exist independently of USB4.
     * USB4_20_GBITPS refers to the capabilities of the USB4 host router.
     * These capabilities are separate; a port might support USB4_20_GBITPS
     * but only support USB3_10_GBITPS (USB 3.2 Gen 2x1) when operating in USB3
     * mode.
     */
    USB4_20_GBITPS = 7,
    /**
     * USB4 Gen 3 speed (40 Gbps).
     *
     * Refers to a USB4 link operating with Gen 3 signaling (20 Gbps)
     * bonded across two lanes.
     */
    USB4_40_GBITPS = 8,
    /**
     * USB4 Gen 4 speed (80 Gbps).
     *
     * Refers to a USB4 symmetric link operating with Gen 4 signaling (40 Gbps)
     * bonded across two lanes.
     */
    USB4_80_GBITPS = 9,
}
