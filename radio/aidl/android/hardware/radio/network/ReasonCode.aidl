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

/**
 * Reason codes for network security alerts.
 *
 * This enum defines the reason codes for network security alerts.
 * These codes are used to communicate the specific reasons for the alert.
 *
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum ReasonCode {
    /**
     * Reason not specified.
     */
    UNSPECIFIED = 0,

    /**
     * A reason for {@link AlertCategory#DOWNGRADE}
     * Network only offered old/weak encryption.
     */
    DOWNGRADE_WEAK_CIPHER_SUITES_OFFERED = 1,
    /**
     * A reason for {@link AlertCategory#DOWNGRADE}
     * Attempts to connect to a higher RAT were rejected.
     */
    DOWNGRADE_HIGHER_RAT_REJECTED = 2,
    /**
     * A reason for {@link AlertCategory#DOWNGRADE}
     * Unusual signal strength differences between RATs.
     */
    DOWNGRADE_SIGNAL_STRENGTH_ANOMALY = 3,
    /**
     * A reason for {@link AlertCategory#DOWNGRADE}
     * Network unexpectedly forced a handover to a lower generation network.
     */
    DOWNGRADE_FORCED_HANDOVER = 4,

    /**
     * A reason for {@link AlertCategory#IMPRISONMENT}
     * Device unable to reselect to any other cell.
     */
    IMPRISONMENT_CELL_RESELECTION_FAILURE = 5,
    /**
     * A reason for {@link AlertCategory#IMPRISONMENT}
     * Serving cell provided no valid neighbor list.
     */
    IMPRISONMENT_NEIGHBOR_LIST_EMPTY_OR_INVALID = 6,
    /**
     * A reason for {@link AlertCategory#IMPRISONMENT}
     * Other potential cells are indicated as barred.
     */
    IMPRISONMENT_BARRING_OF_OTHER_CELLS = 7,
    /**
     * A reason for {@link AlertCategory#IMPRISONMENT}
     * Attempts to move to neighbor cells are rejected.
     */
    IMPRISONMENT_REJECTED_FROM_NEIGHBORS = 8,

    /**
     * A reason for {@link AlertCategory#DOS_NETWORK}
     * Device is being paged at an abnormally high frequency.
     */
    DOS_EXCESSIVE_PAGING_RATE = 9,
    /**
     * A reason for {@link AlertCategory#DOS_NETWORK}
     * Repeated failures and retries in connection setup.
     */
    DOS_CONNECTION_SETUP_FAIL_LOOP = 10,
    /**
     * A reason for {@link AlertCategory#DOS_NETWORK}
     * Flooded with authentication requests from the network.
     */
    DOS_AUTHENTICATION_REQUEST_FLOOD = 11,
    /**
     * A reason for {@link AlertCategory#DOS_NETWORK}
     * Network forcing rapid detach and attach procedures.
     */
    DOS_DETACH_ATTACH_CYCLE = 12,

    /**
     * A reason for {@link AlertCategory#ATTRACTIVE_CELL}
     * Received signal strength is suspiciously high.
     */
    ATTRACTIVE_CELL_VERY_HIGH_RX_LEVEL = 13,
    /**
     * A reason for {@link AlertCategory#ATTRACTIVE_CELL}
     * Cell broadcasting an unexpected PLMN (Network ID).
     */
    ATTRACTIVE_CELL_UNEXPECTED_PLMN_ID = 14,
    /**
     * A reason for {@link AlertCategory#ATTRACTIVE_CELL}
     * Cell does not broadcast any neighbor cell information.
     */
    ATTRACTIVE_CELL_MISSING_NEIGHBOR_INFO = 15,
    /**
     * A reason for {@link AlertCategory#ATTRACTIVE_CELL}
     * Cell parameters match known IMSI catcher signatures.
     */
    ATTRACTIVE_CELL_IMSI_CATCHER_PARAMETERS = 16,

    /**
     * A reason for {@link AlertCategory#JAMMING}
     * High noise levels detected across a wide range of frequencies.
     */
    JAMMING_WIDEBAND_INTERFERENCE = 17,
    /**
     * A reason for {@link AlertCategory#JAMMING}
     * Strong interference on specific operating frequencies.
     */
    JAMMING_NARROWBAND_INTERFERENCE = 18,
    /**
     * A reason for {@link AlertCategory#JAMMING}
     * Sudden and significant drop in Signal-to-Noise Ratio.
     */
    JAMMING_SNR_DEGRADATION = 19,

    /**
     * A reason for {@link AlertCategory#LOCATION_TRACKING}
     * Network requesting location/tracking area updates too frequently.
     */
    LOCATION_FREQUENT_TRACKING_AREA_UPDATES = 20,
    /**
     * A reason for {@link AlertCategory#LOCATION_TRACKING}
     * Detection of non-displayed SMS (potential location ping).
     */
    LOCATION_SILENT_SMS_DETECTED = 21,
    /**
     * A reason for {@link AlertCategory#LOCATION_TRACKING}
     * Frequent paging without subsequent call/SMS/data.
     */
    LOCATION_PAGING_WITHOUT_FOLLOWUP = 22,
    /**
     * A reason for {@link AlertCategory#UNAUTH_SMS}
     * Message failed an integrity check.
     */
    UNAUTH_SMS_INTEGRITY_CHECK_FAILED = 23,
    /**
     * A reason for {@link AlertCategory#UNAUTH_SMS}
     * Expected security elements in SMS transport are missing.
     */
    UNAUTH_SMS_MISSING_SECURITY_HEADERS = 24,
    /**
     * A reason for {@link AlertCategory#UNAUTH_SMS}
     * SMS originated from an untrusted Short Message Entity.
     */
    UNAUTH_SMS_UNTRUSTED_SME = 25,
    /**
     * A reason for {@link AlertCategory#UNAUTH_SMS}
     * Matches a known signature of SMS spoofing.
     */
    UNAUTH_SMS_KNOWN_SPOOFING_METHOD = 26,

    /**
     * A reason for {@link AlertCategory#UNAUTH_EMERGENCY_MSG}
     * Emergency message from a cell that isn't authenticated.
     */
    UNAUTH_EMERGENCY_SOURCE_CELL_NOT_AUTHENTICATED = 27,
}

