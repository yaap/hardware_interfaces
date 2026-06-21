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

import android.hardware.vibrator.Effect;
import android.hardware.vibrator.EffectStrength;

/**
 * Represents a predefined effect.
 *
 * <p>This defines a specific {@link Effect} to be played, modified by an {@link EffectStrength}.
 */
@VintfStability
@FixedSize
parcelable PredefinedEffect {
    /**
     * The type of haptic event to trigger.
     */
    Effect effect;
    /**
     * The intensity of the haptic event to trigger.
     */
    EffectStrength strength;
}
