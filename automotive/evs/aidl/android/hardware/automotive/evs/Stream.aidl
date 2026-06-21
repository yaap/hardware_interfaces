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

import android.hardware.automotive.evs.Rotation;
import android.hardware.automotive.evs.StreamType;
import android.hardware.graphics.common.BufferUsage;
import android.hardware.graphics.common.PixelFormat;

/**
 * Stream:
 *
 * Structure that describes a EVS Camera stream
 *
 * @deprecated EVS functionality and APIs are deprecated.
 *             Use either the Camera2 NDK APIs or the Camera2 Java APIs instead.
 *             For the NDK:
 *             <a
 *             href="https://developer.android.com/ndk/reference/group/camera#acameramanager_getcameracharacteristics">ACameraManager_getCameraCharacteristics</a>
 *             and read the <a
 *             href="https://developer.android.com/ndk/reference/group/camera#acamera_metadata_tag">ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS</a>
 *             tag from <a
 *             href="https://developer.android.com/ndk/reference/group/camera#acamerametadata">ACameraMetadata</a>.
 *             For Java:
 *             {@link android.hardware.camera2.CameraManager#getCameraCharacteristics} and get the
 *             {@link
 *             android.hardware.camera2.CameraCharacteristics#SCALER_STREAM_CONFIGURATION_MAP}
 *             property.
 */
@VintfStability
parcelable Stream {
    /**
     * Stream ID - a non-negative integer identifier for a stream.
     *
     * The identical stream ID must reference the same stream, with the same
     * width/height/format, across consecutive calls to configureStreams.
     *
     * If previously-used stream ID is not used in a new call to
     * configureStreams, then that stream is no longer active. Such a stream ID
     * may be reused in a future configureStreams with a new
     * width/height/format.
     *
     */
    int id;
    /**
     * The type of the stream (input vs output, etc).
     */
    StreamType streamType;
    /**
     * The width in pixels of the buffers in this stream.
     */
    int width;
    /**
     * The height in pixels of the buffers in this stream.
     */
    int height;
    /**
     * The frame rate of this stream in frames-per-second
     */
    int framerate;
    /**
     * The pixel format form the buffers in this stream.
     */
    PixelFormat format;
    /**
     * The gralloc usage flags for this stream, as needed by the consumer of
     * the stream.
     */
    BufferUsage usage;
    /**
     * The required output rotation of the stream.
     *
     * This must be inspected by HAL along with stream with and height.
     */
    Rotation rotation;
}
