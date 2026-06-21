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

package android.hardware.light;

import android.hardware.light.InterpolationType;

/**
 * Represents an effect/animation to be played on a given light.
 *
 * Effects are modeled as a series of color control points, a target frameRate, and an
 * interpolation mode used to smoothen the transitions.
 *
 * Implementations that support light effects should provide interpolation between the color
 * control points. The HAL can use timers or delegate playback to a more specialized hardware.
 *
 * If effects are not supported, the vendor can choose to set the {@link HwLight#maxUpdateHz} to 0
 * to indicate that the hardware does not support fast transitions.
 */
@RustDerive(Clone=true)
@VintfStability
parcelable HwLightEffect {
    /**
     * The ID of the light where the effect will play.
     */
    int lightId;

    /**
     * Number of frames between two given color control points.
     *
     * The size of this array MUST match the size of the {@link #colors} array.
     *
     * When interpolation is NONE, the value of colors[n] is applied frames[n] after the last color
     * was applied.
     *
     * Any negative value is considered an error and will cause the implementation to throw an
     * IllegalArgumentException.
     *
     * Any positive value corresponds to the number of frames (based on the fps) it takes the light
     * to reach the target color, transitioning through interpolated values each frame. This value
     * is relative to the end of the previous frame, so a {10, 10, 10} array means that the light
     * will be at:
     *    - colors[0] at frame 10
     *    - colors[1] at frame 20
     *    - colors[2] at frame 30
     *
     * A value of 0 in this array only makes sense for index0 as a way to provide an initial value
     * for the effect but never in other indices.
     * Applications are not required to provide an initial state and the effect should be played
     * from the last known state, as a way to create smooth transitions.
     */
    int[] frames;

    /**
     * Sequence of target sRGB colors (with alpha set to 0xFF) that will be applied after the
     * corresponding entry in the {@link #frames} array.
     *
     * The size of this array MUST match the size of the {@link #frames} array.
     */
    int[] colors;

    /**
     * Number of times the effect should be played in a loop.
     *
     * A value of 0 is use for "infinite" iterations, which essentially plays the animation until
     * a new state is provided.
     */
    int iterations;

    /**
     * Whether this effect should preempt any previous effect/state the light currently has.
     *
     * When preemptive=false AND the previous light state is a:
     *   - Permanent color: the playback will start immediately. Preemptive has no effect.
     *   - Non-infinite effect: the new effect will start after the previous effect ends all of its
     *     iterations.
     *   - Infinite effect: the playback will start after the current iteration of the effect
     *     finishes.
     *
     * When preemptive=true the effect is applied immediatelly regardless of previous light state.
     *
     * Note that even when the effect preempts the existing state, the effect may not necessarily
     * provide an initial value, and the very first sequence target may be a few frames in the
     * future. In this case, implementations should take the last known state as the base for
     * interpolation to create a fading between the two sequences.
     *   - Last known state is static and no sample with frames=0: last color becomes the starting
     *     color for the interpolator.
     *   - Last known state is interpolating and no sample with frames=0: the starting color for the
     *     interpolator is the last interpolated value from the previous effect.
     */
    boolean preemptive;

    /**
     * Duration of an animation frame for this particular light.
     * <p>
     * This value corresponds to the inverse of the desired frame rate for the effect and has a
     * direct effect on the smoothness of the transitions.
     * <p>
     * If this value is lower than {@link HwLight#minUpdatePeriodMillis} it is considered an error
     * and the implementations should throw an UnsupportedOperationException.
     */
    int framePeriodMillis;

    /**
     * The type of interpolation to apply on every frame.
     */
    InterpolationType interpolationType;
}
