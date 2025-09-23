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

@VintfStability
@FixedSize
union VibrationEffect {
    /**
     * Reserved space for future additions to this union. This ensures
     * backward compatibility.
     */
    long[20] reserved = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    CompositeEffect composite;
    OneShotPrimitive oneShotPrimitive;
    PredefinedEffect predefined;
    PwleV2Primitive pwleV2Primitive;
}
