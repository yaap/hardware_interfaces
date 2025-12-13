/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.tv.mediaquality;

import android.hardware.tv.mediaquality.PictureParameters;
import android.hardware.tv.mediaquality.StreamStatusConfiguration;

/**
 * Represents a complete Picture Profile configuration.
 *
 * <p>A Picture Profile corresponds to a user-selectable mode (e.g., "Cinema", "Sports").
 * It contains a baseline set of parameters and a complete set of variants for all supported
 * stream statuses.
 *
 * <p><b>Lifecycle:</b>
 * <br>The framework sends this profile to the HAL (during boot through sendDefaultPictureProfile).
 * The HAL is expected to <b>cache</b> this data. When the HAL internally detects a stream status
 * change (e.g., input signal switches from SDR to HDR10), it should look up the parameters
 * in its local cache and apply them immediately without any Framework interaction.
 */
@VintfStability
parcelable PictureProfile {
    /**
     * The unique identifier for this picture profile (e.g., ID for "Sports Mode").
     * This ID is used by the framework to request a profile change.
     */
    long pictureProfileId;

    /**
     * The default picture parameters for this profile.
     *
     * <p>These parameters should be cached by the HAL and applied if the detected
     * {@link StreamStatus} does not have a corresponding specific override in
     * {@code streamStatusVariants}.
     */
    PictureParameters parameters;

    /**
     * A collection of parameter overrides for specific stream statuses.
     *
     * <p>This structure serves as the HAL's local lookup table. Upon receiving this profile,
     * the HAL should store these variants. When the stream status changes, the HAL checks
     * its stored copy of these variants to instantly apply the matching {@link PictureParameters}.
     */
    @nullable StreamStatusConfiguration streamStatusConfiguration;
}
