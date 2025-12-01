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

/**
 * Represents a one-shot vibration.
 *
 * <p>One-shot vibrations will vibrate constantly for the specified period of time at the
 * specified amplitude, and then stop.
 */
@VintfStability
@FixedSize
parcelable OneShotPrimitive {
    /**
     * Input amplitude ranges from 0.0 (inclusive) to 1.0 (inclusive), representing the relative
     * input value.
     *
     * <p>Input amplitude linearly maps to output acceleration (e.g., 0.5 amplitude yields half the
     * max acceleration for that frequency).
     *
     * 0.0 represents no output acceleration amplitude
     * 1.0 represents the maximum achievable strength
     */
    float amplitude;
    /**
     * The duration to sustain the vibration at the specified amplitude, in milliseconds.
     */
    int timeMillis;
}
