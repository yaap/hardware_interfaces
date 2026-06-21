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

package android.hardware.radio.network;

import android.hardware.radio.network.NrVopsInfo;
import android.hardware.radio.network.SatelliteTechnology;

/**
 * Information related to NR (5G) network registration status and capabilities.
 *
 * <p>This parcelable provides details such as Voice over PS (VoPS) support on NR
 * and whether the registration is via a satellite network.
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
@SuppressWarnings(value={"redundant-name"})
@RustDerive(Clone=true, Eq=true, PartialEq=true)
parcelable NrRegistrationInfo {
    /**
     * Network capabilities for voice over PS services. This info is valid only on NR network and
     * must be present when the device is camped on NR.
     */
    NrVopsInfo nrVopsInfo;

    /**
     * The type of satellite technology. Modem must report SatelliteTechnology.NONE when camped on
     * a terrestrial network or if it is not aware of the satellite technology.
     */
    SatelliteTechnology satelliteTechnology = SatelliteTechnology.SAT_TECH_NONE;
}
