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

import android.hardware.usb.HorizontalPosition;
import android.hardware.usb.Panel;
import android.hardware.usb.VerticalPosition;

/**
 * Describes the physical location of a USB port.
 * The origin of all panels is the lower-left corner when the user is facing
 * the panel. Position should be described as if looking at the panel head-on.
 *
 * See ACPI specification v6.5, section 6.1.8 - _PLD (Physical Location of
 * Device).
 *
 * Form factor-specific details:
 * - Laptops (clamshell): Follows the definitions in the ACPI specification.
 * - Handheld mobile devices: The front panel is the one with the display. The
 *   origin is the lower-left corner when viewed in portrait orientation
 *   (ACPI specification definition).
 * - Convertibles: Treat as laptops with an approximate 90-degree angle
 *   between the lid and base.
 * - Detachables: Label ports as if the base is attached (laptop definition).
 *   If the ports are on the part of the device with the screen, `lid` should
 *   be set to true.
 */
@VintfStability
parcelable PhysicalLocation {
    /**
     * The panel of the device where the port is located.
     */
    Panel panel = Panel.UNKNOWN;
    /**
     * The horizontal position of the port on the panel.
     */
    HorizontalPosition horizontalPosition = HorizontalPosition.UNKNOWN;
    /**
     * The vertical position of the port on the panel.
     */
    VerticalPosition verticalPosition = VerticalPosition.UNKNOWN;
    /**
     * Whether the port resides on the lid of the device.
     * If there is no lid this should be set to false.
     */
    boolean lid = false;
}
