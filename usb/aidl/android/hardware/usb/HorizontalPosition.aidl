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
 * The horizontal position of the port on the panel.
 */
enum HorizontalPosition {
    /**
     * The horizontal position of the port is unknown.
     */
    UNKNOWN = -1,
    /**
     * The port is on the left side of the panel.
     */
    LEFT = 0,
    /**
     * The port is in the center of the panel.
     */
    CENTER = 1,
    /**
     * The port is on the right side of the panel.
     */
    RIGHT = 2,
}