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
 * Represents the 3D mode of the video.
 */
@VintfStability
enum ThreeDMode {
    /**
     * 3D mode is disabled.
     */
    OFF,

    /**
     * Side-by-side 3D mode, where the left and right views are in the same frame,
     * placed horizontally next to each other.
     */
    SIDE_BY_SIDE,

    /**
     * Top-and-bottom 3D mode, where the left and right views are in the same
     * frame, placed vertically on top of each other.
     */
    TOP_AND_BOTTOM,

    /**
     * Frame packing 3D mode, where left and right eye views are packed into
     * a single frame according to the H.264 Frame Packing Arrangement SEI message.
     * This format typically provides full resolution for each eye.
     */
    FRAME_PACKING,
}
