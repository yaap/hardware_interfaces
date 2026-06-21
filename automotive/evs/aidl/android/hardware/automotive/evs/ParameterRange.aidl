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
 * Represent a valid range of CameraParam
 *
 * @deprecated EVS functionality and APIs are deprecated.
 *             Use the Camera2 NDK API (<a
 *             href="https://developer.android.com/ndk/reference/group/camera#acamerametadata_getconstentry">ACameraMetadata_getConstEntry</a>)
 *             or the Camera2 Java API ({@link android.hardware.camera2.CameraCharacteristics#get})
 *             instead.
 */
@VintfStability
parcelable ParameterRange {
    /**
     * Lower bound of a valid value range
     */
    int min;
    /**
     * Upper bound of a valid value range
     */
    int max;
    /**
     * A value of unit increment
     */
    int step;
}
