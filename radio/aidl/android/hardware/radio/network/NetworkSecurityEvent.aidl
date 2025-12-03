/*
 * Copyright 2025 The Android Open Source Project
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

import android.hardware.radio.network.AlertCategory;
import android.hardware.radio.network.AlertStatus;
import android.hardware.radio.network.ReasonCode;
import android.hardware.radio.RadioTechnology;


/**
 * Represents a single network security event reported by the modem.
 *
 * This structure is used to communicate details about a detected threat,
 * its mitigation status, and specific reasons for the alert, along with
 * network identifiers for context.
 *
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
@RustDerive(Clone=true, Eq=true, PartialEq=true)
parcelable NetworkSecurityEvent {
    /**
     * The general category of the detected security threat.
     * See {@link AlertCategory} for possible values.
     */
    AlertCategory alertCategory = AlertCategory.UNSPECIFIED;

    /**
     * The current status of the threat, indicating whether it was
     * only detected or if a mitigation action was taken.
     * See {@link AlertStatus} for possible values.
     */
    AlertStatus alertStatus = AlertStatus.UNSPECIFIED;

    /**
     * An array of specific reasons that provide more context for the alert.
     * This can be empty if no specific reason is available.
     * See {@link ReasonCode} for possible values.
     */
    ReasonCode[] reasonCodes;

    /**
     * The Cell Identity (CI) of the cell where the event occurred.
     */
    long cellId;

    /**
     * The Physical Cell ID (PCI) of the cell where the event occurred.
     */
    int physicalCellId;

    /**
     * The Absolute Radio Frequency Channel Number (ARFCN) of the cell.
     */
    int arfcn;

    /**
     * The Public Land Mobile Network (PLMN) ID of the network operator.
     */
    String plmn;

    /**
     * The Radio Access Technology (RAT) in use when the event was detected.
     * See {@link RadioTechnology} for possible values.
     */
    RadioTechnology rat = RadioTechnology.UNKNOWN;

    /**
     * A flag indicating if the device is currently in an emergency session
     * (e.g., making an emergency call) when the event occurred.
     */
    boolean isEmergency;
}
