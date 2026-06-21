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
 * Defines a PowerProfile that is not covered by neither the USB Power Delivery
 * Specification nor the USB Type-C Cable and Connector Specification
 */
@VintfStability
parcelable PowerProfileVendor {
    /**
     * Gives the name of the PowerProfile type as defined by the vendor.
     */
    String name;
    /**
     * Describes the minimum voltage allowed by the power profile in millivolts.
     *
     * The value is expected to be 0 or greater is the field is supported. A value
     * of -1 indicates that this field is not implemented.
     */
    int minVoltageMv = -1;
    /**
     * Describes the maximum voltage allowed by the power profile in millivolts.
     *
     * The value is expected to be 0 or greater is the field is supported. A value
     * of -1 indicates that this field is not implemented.
     */
    int maxVoltageMv = -1;
    /**
     * Describes the minimum current allowed by the power profile in milliamps.
     *
     * The value is expected to be 0 or greater is the field is supported. A value
     * of -1 indicates that this field is not implemented.
     */
    int minCurrentMa = -1;
    /**
     * Describes the maximum current allowed by the power profile in milliamps.
     *
     * The value is expected to be 0 or greater is the field is supported. A value
     * of -1 indicates that this field is not implemented.
     */
    int maxCurrentMa = -1;
    /**
     * Describes the maximum power allowed by the power profile in milliwatts.
     *
     * The value is expected to be 0 or greater is the field is supported. A value
     * of -1 indicates that this field is not implemented.
     */
    int maxPowerMw = -1;
}
