/*
 * Copyright 2025 The Android Open Source Project
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

package android.hardware.npu;

import android.hardware.npu.ISchedulingCallback;
import android.hardware.npu.SchedulingConfig;

/**
 * This is used to inform the NPU of the priorities of the applications
 * on the system and receive callbacks related to scheduling decisions.
 */
@VintfStability
interface IScheduling {
    /**
     * Sets priorities based on the passed set of SchedulingConfig
     *
     * @param schedulingConfigs the scheduling configuration for a set of UIDs
     * @throws EX_ILLEGAL_ARGUMENT if parameters of SchedulingConfig are invalid
     */
    void setSchedulingConfigs(in SchedulingConfig[] schedulingConfigs);

    /**
     * Provide an incremental update to the scheduling configs. These will
     * replace an existing config for a given UID or add to the set of configs if
     * there is no existing one for a given UID.
     */
    void updateSchedulingConfigs(in SchedulingConfig[] configs);

    /**
     * Sets a callback to receive scheduling-related information.
     *
     * @param callback The callback instance. Only one callback is allowed. Subsequent
     *                 calls must overwrite the callback set in prior ones.
     */
    void setCallback(in ISchedulingCallback callback);
}
