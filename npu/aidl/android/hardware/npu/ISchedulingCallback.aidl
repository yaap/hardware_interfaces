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

import android.hardware.npu.EndReason;
import android.hardware.npu.StartReason;
import android.hardware.npu.WorkInfo;

/**
 * This callback can be registered via IScheduling#setCallback() to
 * receive information from the NPU related to scheduling decisions.
 */
@VintfStability
oneway interface ISchedulingCallback {
    /**
     * The amount of time to wait in milliseconds for debouncing the
     * onWorkRequested(), onWorkStarted(), and onWorkEnded() events.
     */
    const int DEBOUNCE_DURATION_MS = 50;

    /**
     * Called when a request for work has been received from an app.
     * This will be suppressed if execution for a prior request from
     * the same UID has been completed within DEBOUNCE_DURATION_MS or
     * if there is already ongoing work for the same UID.
     *
     * @param workInfo information describing the work being requested
     */
    void onWorkRequested(in WorkInfo info);

    /**
     * Received when execution has started on a request from an Android app.
     * This will be suppressed if the onWorkRequested() event was
     * suppressed for this WorkInfo.
     *
     * @param workInfo information describing the work being done
     * @param reason the reason associated with the work being started
     */
    void onWorkStarted(in WorkInfo workInfo, in StartReason reason);

    /**
     * Received when exeuction has ended on a request from an Android app. This
     * will only be sent once DEBOUNCE_DURATION_MS has elapsed and another request
     * from the same UID has not been received.
     *
     * @param workInfo information describing the work that was completed
     * @param reason the reason associated with the work ending
     */
    void onWorkEnded(in WorkInfo workInfo, in EndReason reason);
}
