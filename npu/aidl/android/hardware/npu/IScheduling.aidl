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
 *
 * The NPU should make a best-effort to follow the priorities set via `setSchedulingConfigs()`.
 * That means that higher-priority work (which has a lower number) should generally be
 * executed before lower-priority work to the extent that it is possible. The details of this
 * may vary based on the capabilities of the hardware.
 *
 * If the NPU gets work for a UID that does not have an associated
 * SchedulingConfig, it should give it the lowest priority (SchedulingConfig.MAX_PRIORITY),
 * allow direct access, and disallow attribution to other UIDs.
 */
@VintfStability
interface IScheduling {
    /**
     * Sets priorities based on the passed set of SchedulingConfig. This replaces the
     * entire set of configs that may have been passed prior via setSchedulingConfigs() or
     * updateSchedulingConfigs(). For example, passing an empty array will clear all
     * existing configs.
     *
     * @param schedulingConfigs the scheduling configuration for a set of UIDs
     * @throws EX_ILLEGAL_ARGUMENT if parameters of SchedulingConfig are invalid or if
     *                             there are multiple configs for the same UID.
     */
    void setSchedulingConfigs(in SchedulingConfig[] schedulingConfigs);

    /**
     * Provide an incremental update to the scheduling configs. These will
     * replace an existing config for a given UID or add to the set of configs if
     * there is no existing one for a given UID.
     *
     * @param configs the scheduling configuration updates for a set of UIDs
     * @throws EX_ILLEGAL_ARGUMENT if parameters of SchedulingConfig are invalid or if
     *                             there are multiple configs for the same UID.
     */
    void updateSchedulingConfigs(in SchedulingConfig[] configs);

    /**
     * Sets a callback to receive scheduling-related information.
     *
     * @param callback The callback instance. Only one callback is allowed. Subsequent
     *                 calls must overwrite the callback set in prior ones.
     */
    void setCallback(in @nullable ISchedulingCallback callback);
}
