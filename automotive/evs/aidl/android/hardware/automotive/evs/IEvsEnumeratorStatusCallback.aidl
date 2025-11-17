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

import android.hardware.automotive.evs.DeviceStatus;

/**
 * Implemented on client side to receive asynchronous notifications from
 * IEvsEnumreator.
 *
 * @deprecated EVS functionality and APIs are deprecated.
 *             Applications should use the standard Android <a
 *             href="https://developer.android.com/media/camera/camera2">Camera2 API
 *             (android.hardware.camera2)</a> for camera access and management. Use either the
 *             Camera2 NDK APIs (<a
 *             href="https://developer.android.com/ndk/reference/struct/a-camera-manager-availability-listener">ACameraManager_AvailabilityListener</a>)
 *             or Camera2 Java APIs ({@link
 *             android.hardware.camera2.CameraManager.AvailabilityCallback}) instead.
 */
@VintfStability
oneway interface IEvsEnumeratorStatusCallback {
    /**
     * Receives calls from the HAL each time a status of camera devices is
     * changed.
     *
     * @param in status A list of newly updated device status
     *
     * @deprecated EVS functionality and APIs are deprecated.
     */
    void deviceStatusChanged(in DeviceStatus[] status);
}
