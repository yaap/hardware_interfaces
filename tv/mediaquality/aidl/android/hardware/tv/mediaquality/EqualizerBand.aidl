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
 * Represents a single band in an equalizer.
 *
 * An equalizer is composed of multiple bands, each targeting a specific
 * frequency range in the audio spectrum. This parcelable defines the
 * properties of one such band.
 */
@VintfStability
@RustDerive(Clone=true, PartialEq=true)
parcelable EqualizerBand {
    /** The center frequency of this band in Hertz (Hz). */
    int frequencyHz;

    /**
     * The gain (level) for this band in decibel.
     * Range: Determined by capabilities (e.g., -50 to 50).
     */
    int gainDb;

    /**
     * The Quality Factor (Q).
     * Controls the bandwidth of the filter.
     * High Q = Narrow bandwidth. Low Q = Wide bandwidth.
     *
     * Typical range: 0.1(Narrow) to 10.0(wide)
     */
    float qFactor;
}
