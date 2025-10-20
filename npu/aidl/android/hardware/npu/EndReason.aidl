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

/**
 * An enum containing reasons for NPU work ending
 */
@VintfStability
enum EndReason {
    /**
     * Indicates that the work was cancelled by the calling app
     */
    CANCELLED_USER,

    /**
     * Indicates that the work was cancelled because it was preempted
     * by higher-priority work.
     */
    CANCELLED_SYSTEM,

    /**
     * Indicates that the work was paused in order to run higher-priority work
     */
    PAUSED,

    /**
     * Indicates that the work failed
     */
    FAILED,

    /**
     * Indicates that the work was completed
     */
    COMPLETED

}
