/**
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

package android.hardware.wifi.supplicant;
@VintfStability
interface ISupplicantWifiRttControllerEventCallback {
  oneway void onResults(in int cmdId, in android.hardware.wifi.supplicant.RttResult[] results);
  oneway void onContinuousRangingStatusChanged(in int cmdId, in android.hardware.wifi.supplicant.ISupplicantWifiRttControllerEventCallback.ContinuousRangingStatusCode code);
  oneway void onContinuousRangingTerminated(in int cmdId, in android.hardware.wifi.supplicant.ISupplicantWifiRttControllerEventCallback.ContinuousRangingTerminateReasonCode reason);
  @Backing(type="int") @VintfStability
  enum ContinuousRangingStatusCode {
    UNKNOWN = 0,
    PR_RANGE_NEGOTIATION_STARTED = 1,
    PR_RANGE_NEGOTIATION_SUCCEEDED = 2,
    PR_STARTED_RANGE_REQUESTS_ISTA_ROLE = 3,
    PR_STARTED_RANGE_REQUESTS_RSTA_ROLE = 4,
  }
  @Backing(type="int") @VintfStability
  enum ContinuousRangingTerminateReasonCode {
    UNKNOWN = 0,
    TIMEOUT = 1,
    USER_REQUEST = 2,
    ABORT_CONCURRENCY = 3,
    RECEIVED_RTT_TERMINATE = 4,
    PR_RANGE_NEG_FAILED = 5,
  }
}
