/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not us e this file except in compliance with the License.
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

import android.hardware.usb.AltModeCommonData;
import android.hardware.usb.DisplayPortAltModePinAssignment;
import android.hardware.usb.DisplayPortAltModeStatus;
import android.hardware.usb.LinkTrainingStatus;

@VintfStability
union AltModeData {
    /**
     * Holds data necessary to communicate the current DisplayPort
     * Alt Mode status.
     */
    @VintfStability
    parcelable DisplayPortAltModeData {
        /**
         * Indicates the current DisplayPort Alt Mode Status of
         * the port partner acting as a DisplayPort sink.
         */
        DisplayPortAltModeStatus partnerSinkStatus = DisplayPortAltModeStatus.UNKNOWN;
        /**
         * Indicates the current status of the attached DisplayPort
         * Alt Mode cable/adapter.
         */
        DisplayPortAltModeStatus cableStatus = DisplayPortAltModeStatus.UNKNOWN;
        /**
         * Indicates the DisplayPort Alt Mode pin assignment
         * negotiated between the device, port partner, and cable.
         */
        DisplayPortAltModePinAssignment pinAssignment = DisplayPortAltModePinAssignment.NONE;
        /**
         * Indicates DisplayPort Hot Plug Detection (HPD) status for a partner
         * sink device. If true, then a DisplayPort Alt Mode partner sink is
         * connected and powered on, and if false, the partner sink is not
         * powered or no partner sink is connected.
         */
        boolean hpd = false;
        /**
         * Indicates the current status of DisplayPort link training over USB-C
         * for the attached DisplayPort Alt Mode partner sink.
         */
        LinkTrainingStatus linkTrainingStatus = LinkTrainingStatus.UNKNOWN;
        /**
         * Holds alternate mode common data for the DisplayPort Alt Mode.
         */
        @nullable AltModeCommonData commonData;
    }
    DisplayPortAltModeData displayPortAltModeData;

    /**
     * Holds data necessary to communicate the current Thunderbolt 3
     * Alt Mode status.
     */
    @VintfStability
    parcelable Thunderbolt3AltModeData {
        /**
         * Holds alternate mode common data for the Thunderbolt 3 Alt Mode.
         */
        @nullable AltModeCommonData commonData;
    }
    Thunderbolt3AltModeData thunderbolt3AltModeData;

    /**
     * Holds data necessary to communicate the current Vendor
     * Alt Mode status.
     */
    @VintfStability
    parcelable VendorAltModeData {
        /**
         * Indicates the Standard or Vendor ID (SVID) of the Vendor Alt Mode.
         */
        int svid = -1;
        /**
         * Holds alternate mode common data for the Vendor Alt Mode.
         */
        @nullable AltModeCommonData commonData;
    }
    VendorAltModeData vendorAltModeData;
}
