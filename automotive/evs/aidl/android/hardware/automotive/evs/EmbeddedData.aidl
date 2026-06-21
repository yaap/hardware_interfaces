/*
 * Copyright (C) 2023 The Android Open Source Project
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

import android.hardware.common.Ashmem;

/**
 * This structure contains raw embedded data lines and can
 * used to share sensor embedded data that is not a type of
 * histogram or grid statistics.
 *
 * @deprecated EVS functionality and APIs are deprecated.
 *             Applications should use the standard Android <a
 *             href="https://developer.android.com/media/camera/camera2">Camera2 API
 *             (android.hardware.camera2)</a> for camera access and management. Use either the
 *             Camera2 NDK APIs
 *             (<a
 *             href="https://developer.android.com/ndk/reference/group/camera#acameramanager">ACameraManager</a>)
 *             or Camera2 Java APIs ({@link android.hardware.camera2.CameraManager}) instead.
 */
@VintfStability
parcelable EmbeddedData {
    /** Width of the embedded data lines. */
    int widthInBytes;
    /** Number of the embedded data lines. */
    int heightInLines;
    /** Raw embedded data lines. */
    Ashmem data;
}
