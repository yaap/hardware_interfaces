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

import android.hardware.automotive.evs.EmbeddedData;
import android.hardware.automotive.evs.ExposureParameters;
import android.hardware.automotive.evs.GridStatistics;
import android.hardware.automotive.evs.Histogram;
import android.hardware.graphics.common.HardwareBuffer;
import android.hardware.graphics.common.Rect;

/**
 * Structure representing an image buffer through our APIs
 *
 * In addition to the handle to the graphics memory, we need to retain
 * the properties of the buffer for easy reference and reconstruction of
 * an ANativeWindowBuffer object on the remote side of API calls.
 * (Not least because OpenGL expect an ANativeWindowBuffer* for us as a
 * texture via eglCreateImageKHR()).
 *
 * @deprecated EVS functionality and APIs are deprecated.
 *             Applications should use the standard Android <a
 *             href="https://developer.android.com/media/camera/camera2">Camera2 API
 *             (android.hardware.camera2)</a> for camera access and management.
 *             For the NDK:
 *             <a
 *             href="https://developer.android.com/ndk/reference/group/media#aimage">AImage</a>
 *             provides access to the image buffer via <a
 *             href="https://developer.android.com/ndk/reference/group/media#aimage_gethardwarebuffer">AImage_getHardwareBuffer</a>,
 *             and <a
 *             href="https://developer.android.com/ndk/reference/group/camera#acameracapturesession_capturecallback_result">ACameraCaptureSession_captureCallback_result</a>
 *             provides information about the metadata of the parameters used for capturing the
 *             image.
 *             For Java:
 *             {@link android.media.Image} provides access to the image buffer via {@link
 *             android.media.Image#getHardwareBuffer}, and {@link
 *             android.hardware.camera2.CaptureResult#get} provides information about the metadata
 *             of the parameters used for capturing the image.
 */
@VintfStability
parcelable BufferDesc {
    /**
     * Stable AIDL counter part of AHardwareBuffer.  Please see
     * hardware/interfaces/graphics/common/aidl/android/hardware/graphics/common/HardwareBuffer.aidl
     * for more details.
     */
    HardwareBuffer buffer;
    /**
     * The size of a pixel in the units of bytes.
     */
    int pixelSizeBytes;
    /**
     * Opaque value from driver
     */
    int bufferId;
    /**
     * Unique identifier of the physical camera device that produces this buffer.
     */
    @utf8InCpp String deviceId;
    /**
     * Time that this buffer is being filled in the units of microseconds and must be
     * obtained from android::elapsedRealtimeNanos() or its equivalents.
     */
    long timestamp;
    /**
     * Frame metadata.  This is opaque to EvsManager.
     */
    byte[] metadata;
    /**
     * ExposureParameters are expected to be in the ascending
     * order of their exposure time; from the shortest to the
     * longest.  For example, if the imaging sensor output has
     * two exposures, a shorter exposure setting is at index 0
     * and a longer exposure setting is at index 1.
     */
    @nullable ExposureParameters[] exposureSettings;
    /**
     * Histogram statistics calculated on this buffer.  This
     * may contain zero or more histograms.
     */
    @nullable Histogram[] histograms;
    /**
     * Grid statistics calculated on this buffer.  This field
     * also may contain zero or more grid statistics.
     */
    @nullable GridStatistics[] grids;
    /**
     * This may contain raw embedded data lines and can be
     * used to share data other than exposure parameters,
     * histograms, or grid statistics.
     */
    @nullable EmbeddedData embeddedData;
}
