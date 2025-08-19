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

import android.hardware.usb.Bc12Type;

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
}
