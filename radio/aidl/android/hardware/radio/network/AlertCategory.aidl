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
 * Alert category for network security alerts.
 *
 * This enum defines the alert category for network security alerts.
 * These codes are used to communicate the category of the alert.
 *
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum AlertCategory {
    /**
     * Default, unspecified.
     */
    UNSPECIFIED = 0,
    /**
     * Forced downgrade, general.
     */
    DOWNGRADE = 1,
    /**
     * Forced downgrade from 3G to 2G.
     */
    DOWNGRADE_2G = 2,
    /**
     * Forced downgrade from 4G to 3G.
     */
    DOWNGRADE_3G = 3,
    /**
     * Forced downgrade from 5G to 4G.
     */
    DOWNGRADE_4G = 4,
    /**
     * Attempt to lock UE to a cell.
     */
    IMPRISONMENT = 5,
    /**
     * Network Denial of Service.
     */
    DOS_NETWORK = 6,
    /**
     * Suspiciously attractive cell.
     */
    ATTRACTIVE_CELL = 7,
    /**
     * RF Jamming detected.
     */
    JAMMING = 8,
    /**
     * Suspicious location tracking attempts.
     */
    LOCATION_TRACKING = 9,
    /**
     * Network element passed authentication.
     */
    AUTH_PASSED = 10,
    /**
     * Unauthenticated SMS.
     */
    UNAUTH_SMS = 11,
    /**
     * Unauthenticated emergency message.
     */
    UNAUTH_EMERGENCY_MSG = 12,
}
