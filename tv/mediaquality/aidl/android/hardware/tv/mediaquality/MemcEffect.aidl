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
 * Controls the level of Motion Estimation, Motion Compensation (MEMC),
 * also known as motion smoothing.
 * It inserts intermediate frames to make motion appear more fluid, which is especially
 * noticeable for low-frame-rate content.
 */
@VintfStability
enum MemcEffect {
    /**
     * Disables the MEMC effect.
     */
    OFF,

    /**
     * Low intensity MEMC effect.
     */
    LOW,

    /**
     * Medium intensity MEMC effect.
     */
    MIDDLE,

    /**
     * High intensity MEMC effect.
     */
    HIGH,

    /**
     * User-defined MEMC level.
     */
    USER,
}
