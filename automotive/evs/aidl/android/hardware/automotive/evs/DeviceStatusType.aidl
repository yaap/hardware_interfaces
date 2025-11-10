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
 * The status of the devices available through the EVS
 *
 * @deprecated EVS functionality and APIs are deprecated.
 *             Applications should use the standard Android <a
 *             href="https://developer.android.com/media/camera/camera2">Camera2 API
 *             (android.hardware.camera2)</a> for camera access and management.
 *             For the NDK:
 *             <a
 *             href="https://developer.android.com/ndk/reference/group/camera#acameramanager_registeravailabilitycallback">ACameraManager_registerAvailabilityCallback</a>
 *             with <a
 *             href="https://developer.android.com/ndk/reference/struct/a-camera-manager-availability-listener">ACameraManager_AvailabilityListener</a>.
 *             For Java:
 *             {@link android.hardware.camera2.CameraManager#registerAvailabilityCallback} with
 *             {@link android.hardware.camera2.CameraManager.AvailabilityCallback}.
 */
@VintfStability
@Backing(type="int")
enum DeviceStatusType {
    /**
     * A camera device is available and ready to be used.
     */
    CAMERA_AVAILABLE,
    /**
     * A camera device is not available; e.g. disconnected from the system.
     */
    CAMERA_NOT_AVAILABLE,
    /**
     * A display device is available and ready to be used.
     */
    DISPLAY_AVAILABLE,
    /**
     * A display device is not available; e.g. disconnected from the
     * system.
     */
    DISPLAY_NOT_AVAILABLE,
}
