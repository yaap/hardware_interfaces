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
 * Holds data fields that are shared across different alternate mode types.
 *
 * This parcelable is used within the AltModeData union to avoid data
 * duplication.
 */
@VintfStability
parcelable AltModeCommonData {
    /**
     * Indicates whether the Alt Mode is active.
     */
    boolean isActive = false;
    /**
     * Indicates the priority of the Alt Mode.
     *
     * This field is specific to PortStatus#supportedAltModes and is not
     * relevant for PortPartnerStatus#supportedAltModes.
     * It is used to determine the order in which alt mode entry attempts are
     * made, with the lower number indicating a higher priority.
     *
     * A priority or -1 indicates the it is unknown or should not be taken into
     * account for the AltMode (e.g. for port partner or cable alt modes). A
     * valid priority is between 0 and 255.
     */
    int priority = -1;
    /**
     * Indicates the Vendor Defined Object (VDO) of the Alt Mode.
     *
     * Representation Note: Although defined as a signed int in AIDL, this field
     * represents a raw 32-bit unsigned value as defined by the USB Power
     * Delivery specification.
     */
    int vdo = 0;
}
