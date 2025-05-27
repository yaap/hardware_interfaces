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
 * A simple Parcelable containing details related to how a given UID is prioritized.
 */
@VintfStability
parcelable SchedulingConfig {
    /**
     * The Linux UID of the application.
     */
    int uid;

    /**
     * The priority of the application, ranging from 0 to 1000. 0 is the highest priority
     * and 1000 is the lowest. Values outside of this range should not be accepted and
     * methods should throw EX_ILLEGAL_ARGUMENT.
     */
    int priority;

    /**
     * Whether or not this app is able to execute work directly on the NPU. If not, it
     * may only make requests through an intermediary which does have such access.
     */
    boolean hasDirectAccess;

    /**
     * Whether or not this app is able to attribute work to other UIDs or not.
     */
    boolean canAttributeOtherUid;
}
