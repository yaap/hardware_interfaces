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

package android.hardware.tv.mediaquality;

/**
 * Describes the capabilities of the hardware equalizer (EQ).
 *
 * This parcelable provides information about the limitations and supported
 * features of the device's audio equalizer, such as the range of gain
 * adjustments and the specific frequencies that can be manipulated.
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true)
parcelable EqualizerCapabilities {
    /**
     * The minimum supported gain level in decibels (dB). For example, -50.
     * This defines the lowest possible value for the 'gain' in an EqualizerBand.
     */
    int minLevelDb;

    /**
     * The maximum supported gain level in decibels (dB). For example, 50.
     * This defines the highest possible value for the 'gain' in an EqualizerBand.
     */
    int maxLevelDb;

    /**
     * An array of all supported band center frequencies in Hertz (Hz).
     * The user can only create equalizer bands centered at these frequencies.
     */
    int[] supportedFrequenciesHz;

    /**
     * Indicates whether the equalizer supports an adjustable Q factor.
     * If true, the EQ is a parametric EQ, allowing for bandwidth adjustments.
     * If false, it is likely a graphic EQ with fixed bandwidth for each band.
     */
    boolean hasAdjustableQ;
}
