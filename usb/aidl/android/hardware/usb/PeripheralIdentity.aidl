/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law of agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.usb;

import android.hardware.usb.UsbPdRev;

@VintfStability
/**
 * Represents the identity of a USB Power Delivery peripheral (partner or cable).
 * This information is obtained through PD Discover Identity.
 */
parcelable PeripheralIdentity {
    /**
     * The USB Power Delivery revision supported by the peripheral.
     */
    UsbPdRev pdRevision = UsbPdRev.UNKNOWN;
    /**
     * The raw ID Header VDO (Vendor Defined Object).
     *
     * Representation Note: Although defined as a signed int in AIDL, this field
     * represents a raw 32-bit unsigned value as defined by the USB Power Delivery
     * specification.
     */
    int idHeader;
    /**
     * The raw Certification Status VDO.
     *
     * Representation Note: Although defined as a signed int in AIDL, this field
     * represents a raw 32-bit unsigned value as defined by the USB Power Delivery
     * specification.
     */
    int certStat;
    /**
     * The raw Product VDO.
     *
     * Representation Note: Although defined as a signed int in AIDL, this field
     * represents a raw 32-bit unsigned value as defined by the USB Power Delivery
     * specification.
     */
    int productVdo;
    /**
     * The raw Product Type VDOs.
     *
     * Representation Note: Although defined as a signed int in AIDL, this field
     * represents an array of 32-bit unsigned values as defined by the USB Power
     * Delivery specification.
     */
    int[] productTypeVdos = {};
}
