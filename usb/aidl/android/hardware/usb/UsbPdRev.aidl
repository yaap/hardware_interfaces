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
 * USB Power Delivery revision.
 */
enum UsbPdRev {
    /**
     * Unknown USB Power Delivery revision.
     */
    UNKNOWN = -1,
    /**
     * Other revision.
     *
     * The caller should refer to `getInterfaceVersion()` to determine the
     * meaning of OTHER. OTHER is a PD revision not defined in the current HAL
     * version.
     */
    OTHER = 0,
    /**
     * USB Power Delivery 2.0.
     */
    PD2P0 = 1,
    /**
     * USB Power Delivery 3.0.
     */
    PD3P0 = 2,
    /**
     * USB Power Delivery 3.1.
     */
    PD3P1 = 3,
    /**
     * USB Power Delivery 3.2.
     */
    PD3P2 = 4,
}
