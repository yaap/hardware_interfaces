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

import android.hardware.usb.AltModeData;
import android.hardware.usb.Bc12Type;
import android.hardware.usb.PeripheralIdentity;
import android.hardware.usb.PortDataRole;
import android.hardware.usb.PortPowerRole;
import android.hardware.usb.PowerProfile;

/**
 * Indicates the status of the USB port partner (i.e. the USB charger, accessory, host) to which
 * the local USB port is connected to.
 */
@VintfStability
parcelable PortPartnerStatus {
    /**
     * Indicates the current BC 1.2 type of the port partner
     */
    Bc12Type bc12Type = Bc12Type.UNKNOWN;
    /**
     * Lists the port partner's sink power profiles
     */
    @nullable PowerProfile[] sinkPowerProfiles;
    /**
     * Lists the port partner's source power profiles
     */
    @nullable PowerProfile[] sourcePowerProfiles;
    /**
     * A list of alternate modes supported by the partner.
     */
    AltModeData[] supportedAltModes = {};
    /**
     * The active power role of the partner.
     */
    PortPowerRole activePowerRole = PortPowerRole.NONE;
    /**
     * The active data role of the partner.
     */
    PortDataRole activeDataRole = PortDataRole.NONE;
    /**
     * The identity of the partner. This can be null if the partner does not
     * report identity or if the information has not been retrieved yet.
     */
    @nullable PeripheralIdentity identity;
}
