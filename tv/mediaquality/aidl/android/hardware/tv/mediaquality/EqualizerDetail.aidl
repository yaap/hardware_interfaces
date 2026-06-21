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

import android.hardware.tv.mediaquality.EqualizerBand;

@VintfStability
@RustDerive(Clone=true, PartialEq=true)
parcelable EqualizerDetail {
    /**
     * Levels for a set of predefined equalizer bands. The value for each band
     * is in a range from -50 to 50.
     * The bands are: 120Hz, 500Hz, 1.5kHz, 5kHz, and 10kHz.
     */
    int band120Hz;
    int band500Hz;
    int band1_5kHz;
    int band5kHz;
    int band10kHz;
    /**
     * An array for custom equalizer bands, providing more flexibility than the
     * predefined set. This is intended as an extension.
     */
    EqualizerBand[] bands = {};
}
