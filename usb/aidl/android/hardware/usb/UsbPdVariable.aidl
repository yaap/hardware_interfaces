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

package android.hardware.usb;

/**
 * Defines a PowerProfile for a Variable Supply PDO as defined by the USB Power
 * Delivery Specification
 */
@VintfStability
parcelable UsbPdVariable {
    /**
     * Describes the maximum voltage allowed by the power profile in millivolts.
     * This field must be exactly the value of the "Maximum Voltage" field in the
     * Variable Supply PDO.
     *
     * The value is expected to be 0 or greater.
     */
    int maxVoltageMv;
    /**
     * Describes the minimum voltage allowed by the power profile in millivolts.
     * This field must be exactly the value of the "Minimum Voltage" field in the
     * Variable Supply PDO.
     *
     * The value is expected to be 0 or greater.
     */
    int minVoltageMv;
    /**
     * Describes the maximum current allowed by the power profile in milliamps.
     * This field must be exactly the value of the "Maximum Current" field in the
     * Variable Supply PDO.
     *
     * The value is expected to be 0 or greater.
     */
    int maxCurrentMa;
}
