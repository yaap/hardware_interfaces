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

@VintfStability
@Backing(type="int")
/**
 * The panel of the device where the port is located.
 */
enum Panel {
    /**
     * The panel of the device where the port is located is unknown.
     */
    UNKNOWN = -1,
    /**
     * The port is on the top panel of the device.
     */
    TOP = 0,
    /**
     * The port is on the bottom panel of the device.
     */
    BOTTOM = 1,
    /**
     * The port is on the left panel of the device.
     */
    LEFT = 2,
    /**
     * The port is on the right panel of the device.
     */
    RIGHT = 3,
    /**
     * The port is on the front panel of the device.
     */
    FRONT = 4,
    /**
     * The port is on the back panel of the device.
     */
    BACK = 5,
}