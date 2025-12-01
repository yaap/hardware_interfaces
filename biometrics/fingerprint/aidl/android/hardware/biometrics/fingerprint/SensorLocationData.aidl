/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.biometrics.fingerprint;

import android.hardware.biometrics.fingerprint.location.HomeButtonLocation;
import android.hardware.biometrics.fingerprint.location.PowerButtonDisplayLocation;
import android.hardware.biometrics.fingerprint.location.PowerButtonPhysicalLocation;
import android.hardware.biometrics.fingerprint.location.RearLocation;
import android.hardware.biometrics.fingerprint.location.StandaloneLocation;
import android.hardware.biometrics.fingerprint.location.UnderDisplayLocation;

/**
 * Holds the mutually exclusive location data for a sensor.
 * A new HAL must set exactly one of these fields based on the
 * sensor's FingerprintSensorType.
 * @hide
 */
@VintfStability
union SensorLocationData {
    UnderDisplayLocation underDisplayLocation;
    PowerButtonDisplayLocation powerButtonDisplayLocation;
    PowerButtonPhysicalLocation powerButtonPhysicalLocation;
    StandaloneLocation standaloneLocation;
    HomeButtonLocation homeButtonLocation;
    RearLocation rearLocation;
}
