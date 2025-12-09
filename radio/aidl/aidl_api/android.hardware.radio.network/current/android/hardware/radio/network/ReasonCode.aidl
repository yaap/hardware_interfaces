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
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.radio.network;
/* @hide */
@Backing(type="int") @JavaDerive(toString=true) @VintfStability
enum ReasonCode {
  UNSPECIFIED = 0,
  DOWNGRADE_WEAK_CIPHER_SUITES_OFFERED = 1,
  DOWNGRADE_HIGHER_RAT_REJECTED = 2,
  DOWNGRADE_SIGNAL_STRENGTH_ANOMALY = 3,
  DOWNGRADE_FORCED_HANDOVER = 4,
  IMPRISONMENT_CELL_RESELECTION_FAILURE = 5,
  IMPRISONMENT_NEIGHBOR_LIST_EMPTY_OR_INVALID = 6,
  IMPRISONMENT_BARRING_OF_OTHER_CELLS = 7,
  IMPRISONMENT_REJECTED_FROM_NEIGHBORS = 8,
  DOS_EXCESSIVE_PAGING_RATE = 9,
  DOS_CONNECTION_SETUP_FAIL_LOOP = 10,
  DOS_AUTHENTICATION_REQUEST_FLOOD = 11,
  DOS_DETACH_ATTACH_CYCLE = 12,
  ATTRACTIVE_CELL_VERY_HIGH_RX_LEVEL = 13,
  ATTRACTIVE_CELL_UNEXPECTED_PLMN_ID = 14,
  ATTRACTIVE_CELL_MISSING_NEIGHBOR_INFO = 15,
  ATTRACTIVE_CELL_IMSI_CATCHER_PARAMETERS = 16,
  JAMMING_WIDEBAND_INTERFERENCE = 17,
  JAMMING_NARROWBAND_INTERFERENCE = 18,
  JAMMING_SNR_DEGRADATION = 19,
  LOCATION_FREQUENT_TRACKING_AREA_UPDATES = 20,
  LOCATION_SILENT_SMS_DETECTED = 21,
  LOCATION_PAGING_WITHOUT_FOLLOWUP = 22,
  UNAUTH_SMS_INTEGRITY_CHECK_FAILED = 23,
  UNAUTH_SMS_MISSING_SECURITY_HEADERS = 24,
  UNAUTH_SMS_UNTRUSTED_SME = 25,
  UNAUTH_SMS_KNOWN_SPOOFING_METHOD = 26,
  UNAUTH_EMERGENCY_SOURCE_CELL_NOT_AUTHENTICATED = 27,
}
