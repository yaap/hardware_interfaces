/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.automotive.evs;

/**
 * Types of informative streaming events
 *
 * @deprecated EVS functionality and APIs are deprecated.
 *             Use the Camera2 NDK API (<a
 *             href="https://developer.android.com/ndk/reference/group/camera#acameracapturesession_capturecallbacksv2">ACameraCaptureSession_captureCallbacksV2</a>)
 *             or the Camera2 Java API ({@link
 *             android.hardware.camera2.CameraCaptureSession.CaptureCallback}) instead.
 */
@VintfStability
@Backing(type="int")
enum EvsEventType {
    /**
     * Video stream is started
     */
    STREAM_STARTED = 0,
    /**
     * Video stream is stopped
     */
    STREAM_STOPPED,
    /**
     * Video frame is dropped
     */
    FRAME_DROPPED,
    /**
     * Timeout happens
     */
    TIMEOUT,
    /**
     * Camera parameter is changed; payload contains a changed parameter ID and
     * its value
     */
    PARAMETER_CHANGED,
    /**
     * Master role has become available
     */
    MASTER_RELEASED,
    /**
     * Any other erroneous streaming events
     */
    STREAM_ERROR,
}
