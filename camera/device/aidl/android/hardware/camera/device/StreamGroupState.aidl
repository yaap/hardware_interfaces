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

package android.hardware.camera.device;

/**
 * StreamGroupState:
 *
 * Message contents for MsgType::SHUTTER related to multi-resolution
 * outputs when `concurrentGroup` is set to true for the output group.
 */
@VintfStability
parcelable StreamGroupState {
    /**
     * The group id of the stream group. A stream group is a group of streams
     * used for multi-resolution output streams.
     *
     * see Stream::groupId for details.
     */
    int groupId;

    /**
     * The streams within the stream group that are outputing
     * buffers for a particular capture. The stream Ids must all belong to the
     * same groupId specified above.
     */
    int[] activeStreamIds = {};
}
