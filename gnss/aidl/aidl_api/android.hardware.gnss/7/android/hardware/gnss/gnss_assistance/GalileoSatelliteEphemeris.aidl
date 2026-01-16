/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.gnss.gnss_assistance;
/* @hide */
@VintfStability
parcelable GalileoSatelliteEphemeris {
  int svid;
  android.hardware.gnss.gnss_assistance.GalileoSatelliteEphemeris.GalileoSatelliteClockModel[] satelliteClockModel;
  android.hardware.gnss.gnss_assistance.KeplerianOrbitModel satelliteOrbitModel;
  android.hardware.gnss.gnss_assistance.GalileoSatelliteEphemeris.GalileoSvHealth svHealth;
  android.hardware.gnss.gnss_assistance.SatelliteEphemerisTime satelliteEphemerisTime;
  @VintfStability
  parcelable GalileoSatelliteClockModel {
    long timeOfClockSeconds;
    double af0;
    double af1;
    double af2;
    double bgdSeconds;
    double sisaMeters;
    android.hardware.gnss.gnss_assistance.GalileoSatelliteEphemeris.GalileoSatelliteClockModel.SatelliteClockType satelliteClockType;
    @Backing(type="int") @VintfStability
    enum SatelliteClockType {
      UNDEFINED = 0,
      GALILEO_FNAV_CLOCK = 1,
      GALILEO_INAV_CLOCK = 2,
    }
  }
  @VintfStability
  parcelable GalileoSvHealth {
    android.hardware.gnss.gnss_assistance.GalileoSatelliteEphemeris.GalileoSvHealth.GalileoHealthDataVaidityType dataValidityStatusE1b;
    android.hardware.gnss.gnss_assistance.GalileoSatelliteEphemeris.GalileoSvHealth.GalileoHealthStatusType signalHealthStatusE1b;
    android.hardware.gnss.gnss_assistance.GalileoSatelliteEphemeris.GalileoSvHealth.GalileoHealthDataVaidityType dataValidityStatusE5a;
    android.hardware.gnss.gnss_assistance.GalileoSatelliteEphemeris.GalileoSvHealth.GalileoHealthStatusType signalHealthStatusE5a;
    android.hardware.gnss.gnss_assistance.GalileoSatelliteEphemeris.GalileoSvHealth.GalileoHealthDataVaidityType dataValidityStatusE5b;
    android.hardware.gnss.gnss_assistance.GalileoSatelliteEphemeris.GalileoSvHealth.GalileoHealthStatusType signalHealthStatusE5b;
    @Backing(type="int") @VintfStability
    enum GalileoHealthDataVaidityType {
      DATA_VALID = 0,
      WORKING_WITHOUT_GUARANTEE = 1,
    }
    @Backing(type="int") @VintfStability
    enum GalileoHealthStatusType {
      OK = 0,
      OUT_OF_SERVICE = 1,
      EXTENDED_OPERATION_MODE = 2,
      IN_TEST = 3,
    }
  }
}
