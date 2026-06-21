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

import android.hardware.vibrator.CompositeEffect;
import android.hardware.vibrator.OneShotPrimitive;
import android.hardware.vibrator.PredefinedEffect;
import android.hardware.vibrator.PwleV2Primitive;
import android.hardware.vibrator.VendorEffect;

/**
 * VibrationEffectContent represent an atomic part of a VibrationEffect.
 *
 * It has a fixed size and can be used to send vibration effects across Fast Message Queues (FMQs).
 *
 * Each content type represents one of the following vibrate APIs:
 * <ul>
 * <li>OneShotPrimitive: IVibrator.on followed by IVibrator.setAmplitude.
 * <li>PredefinedEffect: IVibrator.perform.
 * <li>CompositeEffect: IVibrator.compose (single primitive + scale + delay).
 * <li>PwleV2Primitive: IVibrator.composePwleV2 (single envelope primitive).
 * </ul>
 */
@VintfStability
@FixedSize
union VibrationEffectContent {
    // Reserved space for future additions to this union. This ensures backward compatibility.
    byte[32] reserved = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0};
    CompositeEffect composite;
    OneShotPrimitive oneShotPrimitive;
    PredefinedEffect predefined;
    PwleV2Primitive pwleV2Primitive;
}
