/*
 * Copyright (C) 2026 The Android Open Source Project
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

package android.hardware.radio.data;

import android.hardware.radio.AccessNetwork;
import android.hardware.radio.data.DataNetworkState;
import android.hardware.radio.data.TransportType;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
@RustDerive(Clone=true, Eq=true, PartialEq=true)
parcelable ImsDataNetworkInfo {
    /**
     * The access network type.
     */
    AccessNetwork accessNetwork = AccessNetwork.UNKNOWN;

    /**
     * The data network connection state.
     */
    DataNetworkState dataNetworkState = DataNetworkState.UNKNOWN;

    /**
     * The physical transport type of the data network.
     */
    TransportType physicalTransportType = TransportType.WWAN;

    /**
     * The logic modem ID while the physical transport type is WWAN. If the physical transport type
     * is WLAN, this modem ID will be -1.
     */
    int physicalNetworkModemId;
}
