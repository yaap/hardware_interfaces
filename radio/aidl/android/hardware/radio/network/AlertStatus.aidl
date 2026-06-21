/*
 * Copyright 2025 The Android Open Source Project
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

 package android.hardware.radio.network;

/**
 * Alert status for network security alerts.
 *
 * This enum defines the alert status for network security alerts.
 * These codes are used to communicate the status of the alert.
 *
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum AlertStatus {
    /**
     * Default unspecified. To be used if the modem does not support the alert.
     */
    UNSPECIFIED = 0,
    /**
     * Modem is not detecting this alert.
     */
    NOT_DETECTED = 1,
    /**
     * Modem detected a threat.
     */
    DETECTED = 2,
    /**
     * Modem detected and blocked cell.
     */
    MITIGATED_CELL_BARRED = 3,
    /**
     * Modem detected and deprioritized.
     */
    MITIGATED_CELL_DEPRIORITIZED = 4,
    /**
     * Modem detected and took unspecified action.
     */
    MITIGATED_UNSPECIFIED = 5,
}
