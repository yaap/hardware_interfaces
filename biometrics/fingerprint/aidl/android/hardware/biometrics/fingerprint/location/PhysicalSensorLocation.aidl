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

package android.hardware.biometrics.fingerprint.location;

/**
 * Represents the physical location of a sensor on a device.
 * @hide
 */
@VintfStability
@Backing(type="byte")
enum PhysicalSensorLocation {
    // Sensor location is unknown
    UNKNOWN,
    // Sensor is located at the bottom left of keyboard
    KEYBOARD_BOTTOM_LEFT,
    // Sensor is located at the bottom right of keyboard
    KEYBOARD_BOTTOM_RIGHT,
    // Sensor is located at the top right of keyboard
    KEYBOARD_TOP_RIGHT,
    // Sensor is located on the right side of device
    RIGHT_SIDE,
    // Sensor is located on the left side of device
    LEFT_SIDE,
    // Sensor is located to the left of the power button at the top right of the keyboard
    LEFT_OF_POWER_BUTTON_TOP_RIGHT,
    // Sensor is location on power button at the top right key of the keyboard.
    POWER_BUTTON_TOP_RIGHT_KEY
}
