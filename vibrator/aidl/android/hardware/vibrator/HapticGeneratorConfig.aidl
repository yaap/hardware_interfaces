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

package android.hardware.vibrator;
import android.media.audio.common.AudioConfigBase;

@VintfStability
parcelable HapticGeneratorConfig {
    /**
     * The desired audio format for the PCM data that will be produced by this
     * session. The HAL should reject the session if it cannot produce data
     * in this format.
     */
    AudioConfigBase audioFormat;

    /**
     * Vendor extension point for vibration effect PCM conversion.
     */
    ParcelableHolder vendorExtension;
}
