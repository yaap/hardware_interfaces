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

package android.hardware.bluetooth.audio;

import android.hardware.bluetooth.audio.AudioContext;

/**
 * Used for ISO Parameter Update to provide the specific update latency
 * setting from Audio HAL.
 */
@VintfStability
parcelable LeAudioUpdateLatencySetting {
    /*
     * The default suggested update latency in units of milliseconds
     */
    int defaultSuggestedLatencyMs;

    /*
     * The specific suggested update latency for the specific condition.
     */
    @nullable SuggestedLatencyRule[] suggestedLatencyRules;

    @VintfStability
    parcelable ConfigChangeConditionFlags {
        /**
         * Set for the update with transport latency change
         */
        const int WITH_TRANSPORT_LATENCY_CHANGE = 0x0001;
        /**
         * Indicates that codec configuration parameters have changed.
         * Note: This flag does not include transport latency changes. Use this flag to
         * distinguish between the following scenarios:
         *
         * Config changed AND Latency changed:
         *       WITH_CONFIG_PARAMETERS_CHANGE | WITH_TRANSPORT_LATENCY_CHANGE
         * Config unchanged BUT Latency changed:  WITH_TRANSPORT_LATENCY_CHANGE
         * Config changed WITHOUT Latency changed: WITH_CONFIG_PARAMETERS_CHANGE
         */
        const int WITH_CONFIG_PARAMETERS_CHANGE = 0x0002;
        /**
         * Set for the update with codec type change
         */
        const int WITH_CODEC_TYPE_CHANGE = 0x0004;
        /**
         * Set for the update with CIS direction change
         */
        const int WITH_CIS_DIRECTIONS_CHANGE = 0x0008;
        /**
         * Set for the update with the phy
         */
        const int WITH_PHY_CHANGE = 0x0010;

        int bitmask;
    }
    @VintfStability
    parcelable SuggestedLatencyRule {
        int suggestedLatencyMs;
        ConfigChangeConditionFlags configChangeConditionFlags;
    }
}
