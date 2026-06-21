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

import android.hardware.npu.SchedulingConfig;
import android.hardware.npu.Uuid;

/**
 * A simple Parcelable containing details related to how a given UID is prioritized.
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true)
parcelable WorkInfo {
    /**
     * An identifier for this work. This must be monotonically increasing.
     */
    int id;

    /**
     * An identifier that indicates that this work is part of a larger effort
     * which is comprised of multiple inferences. This value will be the same
     * for all such inferences. May be null in the cases where there is no
     * such relationship or if the application did not specify it.
     */
    @nullable Uuid groupId;

    /**
     * The Linux UID of the requesting application. Obtained by the
     * HAL via IPCThreadState::getCallingUid() or similar secure mechanism.
     */
    int uid;

    /**
     * The Linux PID of the requesting application.
     */
    int debugPid;

    /**
     * The Linux UID of the original app that requested the execution. May be
     * the same as 'uid'.
     *
     * In cases where an intermediary service is requesting work on behalf of another
     * app, the 'originalUid' would be the caller to that service and 'uid'
     * would be the uid of the intermediary. How 'originalUid' is communicated to the
     * HAL is unspecified, but it must only allow applications to
     * do this attribution if it has received a 'SchedulingConfig' with 'canAttributeOtherUid'
     * set to 'true' for the uid of the intermediary service.
     *
     * In the case where an app is requesting work for itself, 'originalUid' and 'uid' would
     * be the same.
     */
    int originalUid;

    /**
     * A string identifying the feature that this work is associated with, e.g.
     * "com.foo.text_summarization". This can be used for debugging or metrics. May be null.
     */
    @nullable String debugFeatureId;

    /**
     * The priority for this specific work, ranging from {@link SchedulingConfig#MIN_PRIORITY}
     * to {@link SchedulingConfig#MAX_PRIORITY}.
     */
    int jobPriority;

    /**
     * The effective priority for this work, combining (via addition) the UID priority supplied via
     * {@link IScheduling#setSchedulingConfigs} and 'jobPriority'. This ranges from
     * {@link SchedulingConfig#MIN_PRIORITY} to {@link SchedulingConfig#MAX_PRIORITY} * 2.
     */
    int effectivePriority;

    /**
     * This is the CLOCK_MONOTONIC time in milliseconds at which the event occurred when
     * this parcelable is sent via IScheduling::onWorkStarted() and similar notifications.
     */
    long timestampMs;

    /**
     * An identifier for the NPU that is involved with the work. The first device must be 0,
     * and subsequent NPUs (if any) increase serially from there (0, 1, 2, etc.).
     */
    int deviceNumber;
}
