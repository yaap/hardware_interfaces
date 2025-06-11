/*
 * Copyright (C) 2024 The Android Open Source Project
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

@VintfStability
enum PictureQualityEventType {
    /* No status change */
    NONE,

    /**
     * Black bar detection Event.
     *
     * TV has detected or lost track of black bars, potentially triggering a change in aspect
     * ratio.
     */
    BBD_RESULT,

    /**
     * Video delay change event.
     *
     * This signifies a change in the video processing delay, might due to enabling or disabling
     * certain picture quality features.
     */
    VIDEO_DELAY_CHANGE,

    /**
     * Capture point change event.
     *
     * A change in video processing pipeline the image is being captured for display. Changes here
     * relates to switching between different video sources.
     */
    CAPTUREPOINT_INFO_CHANGE,

    /**
     * Video path change event.
     *
     * Indicates a change in the video signal path. This could involve switching between
     * different input sources.
     */
    VIDEOPATH_CHANGE,

    /**
     * Extra frame change event.
     *
     * Some TVs use techniques like frame interpolation (inserting extra frames) to smooth motion.
     * Change means the function is turned on or off.
     */
    EXTRA_FRAME_CHANGE,

    /**
     * Dolby Vision IQ change event.
     *
     * Dolby Vision IQ is a technology that adjusts HDR video based on the ambient light in the
     * room. A change means the function is turned on or off.
     */
    DOLBY_IQ_CHANGE,

    /**
     * Dolby Vision audio processing object change event.
     *
     * This event might be triggered by changes in audio settings that affect the picture quality,
     * such as enabling or disabling a feature that synchronizes audio and video processing.
     */
    DOLBY_APO_CHANGE,
}
