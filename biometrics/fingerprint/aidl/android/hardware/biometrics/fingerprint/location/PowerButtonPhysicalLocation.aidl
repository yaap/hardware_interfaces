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

package android.hardware.biometrics.fingerprint.location;

import android.hardware.biometrics.fingerprint.location.PhysicalSensorLocation;

/**
 * Defines a physical sensor location for chassis-mounted POWER_BUTTON
 * fingerprint sensors (e.g., on a phone side or laptop deck).
 *
 * <p>Note: For sensors positioned on the display bezel that require a UI indicator
 * relative to the screen pixels, use {@link PowerButtonDisplayLocation} instead.
 * @hide
 */
@VintfStability
parcelable PowerButtonPhysicalLocation {
    PhysicalSensorLocation location;
}
