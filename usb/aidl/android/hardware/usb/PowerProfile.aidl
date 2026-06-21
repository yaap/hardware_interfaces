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

import android.hardware.usb.PowerProfileVendor;
import android.hardware.usb.TypecDefault;
import android.hardware.usb.UsbPdBattery;
import android.hardware.usb.UsbPdFixed;
import android.hardware.usb.UsbPdSprAvs;
import android.hardware.usb.UsbPdSprPps;
import android.hardware.usb.UsbPdVariable;

/**
 * Holds the relevant power profile information
 */
@VintfStability
union PowerProfile {
    TypecDefault typecDefaultProfile;
    /**
     * 1.5 A @ 5V as defined by the USB Type-C Cable and Connector Specification
     */
    boolean typec15AProfile;
    /**
     * 3.0 A @ 5V as defined by the USB Type-C Cable and Connector Specification
     */
    boolean typec30AProfile;
    UsbPdFixed fixedProfile;
    UsbPdVariable variableProfile;
    UsbPdBattery batteryProfile;
    UsbPdSprPps sprPpsProfile;
    UsbPdSprAvs sprAvsProfile;
    PowerProfileVendor vendorProfile;
}
