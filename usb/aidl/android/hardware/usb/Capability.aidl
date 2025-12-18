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
 * Capability of a USB port.
 */
enum Capability {
    /**
     * Port supports USB host mode.
     */
    HOST_MODE = 0,
    /**
     * Port supports USB device mode. This is also known as peripheral mode.
     */
    DEVICE_MODE = 1,
    /**
     * Port supports xHCI debug capability (DbC).
     *
     * ref: xHCI spec. rev 1.2, section 7.6 Debug Capability.
     */
    DBC = 2,
    /**
     * Port supports USB Type-C Debug Accessory Mode (SuzyQ).
     */
    TYPEC_DEBUG_ACCESSORY_MODE = 3,
}
