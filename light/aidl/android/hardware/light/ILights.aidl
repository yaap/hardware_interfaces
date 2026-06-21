/*
 * Copyright (C) 2020 The Android Open Source Project
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

import android.hardware.light.HwLight;
import android.hardware.light.HwLightEffect;
import android.hardware.light.HwLightState;

/**
 * Allows controlling logical lights/indicators, mapped to LEDs in a
 * hardware-specific manner by the HAL implementation.
 */
@VintfStability
interface ILights {
    /**
     * Set light identified by id to the provided state.
     *
     * If control over an invalid light is requested, this method must throw an
     * UnsupportedOperationException. Control over supported lights is done on a device-specific
     * best-effort basis and unsupported sub-features will not be reported.
     *
     * @param id ID of logical light to set as returned by getLights()
     * @param state describes what the light should look like.
     */
    void setLightState(in int id, in HwLightState state);

    /**
     * Discover what lights are supported by the HAL implementation.
     *
     * @return List of available lights
     */
    HwLight[] getLights();

    /**
     * Plays light effects on one or more lights.
     *
     * If control over an invalid light is requested, this method throws an
     * UnsupportedOperationException. Control over supported lights is done on a device-specific
     * best-effort basis and unsupported sub-features will not be reported.
     *
     * If the effect is ill formed or requests unsupported frame rates this method must throw an
     * IllegalArgumentException.
     *
     * @param effects Effects that should be applied to the different lights. The id of the light it
     *                should be played on is specified in the light effect itself.
     */
    void setLightEffects(in HwLightEffect[] effects);
}
