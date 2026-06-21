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
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.automotive.evs;
/**
 * @deprecated EVS functionality and APIs are deprecated. Applications should use the standard Android <a href="https://developer.android.com/media/camera/camera2">Camera2 API (android.hardware.camera2)</a> for camera access and management. Use either the Camera2 NDK APIs (<a href="https://developer.android.com/ndk/reference/group/camera#acameramanager">ACameraManager</a>) or Camera2 Java APIs ({@link android.hardware.camera2.CameraManager}) instead.
 */
@VintfStability
interface IEvsEnumerator {
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use the Camera2 NDK API (<a href="https://developer.android.com/ndk/reference/group/camera#acameradevice_close">ACameraDevice_close</a>) or the Camera2 Java API ({@link android.hardware.camera2.CameraDevice#close}) instead.
   */
  void closeCamera(in android.hardware.automotive.evs.IEvsCamera carCamera);
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use {@link android.view.ViewManager#removeView} instead.
   */
  void closeDisplay(in android.hardware.automotive.evs.IEvsDisplay display);
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use <a href="https://source.android.com/docs/automotive/camera/acs/camera2-migration#ultrasonics-apis">Ultrasonics VHAL properties</a> instead.
   */
  void closeUltrasonicsArray(in android.hardware.automotive.evs.IEvsUltrasonicsArray evsUltrasonicsArray);
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use either the Camera2 NDK APIs or the Camera2 Java APIs instead. For the NDK: <a href="https://developer.android.com/ndk/reference/group/camera#acameramanager_getcameraidlist">ACameraManager_getCameraIdList</a> and then <a href="https://developer.android.com/ndk/reference/group/camera#acameramanager_getcameracharacteristics">ACameraManager_getCameraCharacteristics</a> to retrieve individual camera details. For Java: {@link android.hardware.camera2.CameraManager#getCameraIdList} and then {@link android.hardware.camera2.CameraManager#getCameraCharacteristics} to retrieve individual camera details.
   */
  android.hardware.automotive.evs.CameraDesc[] getCameraList();
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use {@link android.hardware.display.DisplayManager#getDisplays} instead.
   */
  byte[] getDisplayIdList();
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use {@link android.view.Display#getState} instead.
   */
  android.hardware.automotive.evs.DisplayState getDisplayState();
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use either the Camera2 NDK APIs or the Camera2 Java APIs instead. For the NDK: <a href="https://developer.android.com/ndk/reference/group/camera#acameramanager_getcameracharacteristics">ACameraManager_getCameraCharacteristics</a> and read the <a href="https://developer.android.com/ndk/reference/group/camera#acamera_metadata_tag">ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS</a> tag from <a href="https://developer.android.com/ndk/reference/group/camera#acamerametadata">ACameraMetadata</a>. For Java: {@link android.hardware.camera2.CameraManager#getCameraCharacteristics} and get the {@link android.hardware.camera2.CameraCharacteristics#SCALER_STREAM_CONFIGURATION_MAP} property.
   */
  android.hardware.automotive.evs.Stream[] getStreamList(in android.hardware.automotive.evs.CameraDesc description);
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use <a href="https://source.android.com/docs/automotive/camera/acs/camera2-migration#ultrasonics-apis"> Ultrasonics VHAL properties</a> instead.
   */
  android.hardware.automotive.evs.UltrasonicsArrayDesc[] getUltrasonicsArrayList();
  /**
   * @deprecated EVS functionality and APIs are deprecated.
   */
  boolean isHardware();
  /**
   * @deprecated EVS functionality and APIs are deprecated. The EVS {@link #openCamera} combines opening the device and configuring a single stream; Camera2 separates them. To open a device with the Camera2 NDK or Java API: <ol> <li>Select one of these modes: <ul> <li> Exclusive mode, use the NDK API (<a href="https://developer.android.com/ndk/reference/group/camera#acameramanager_opencamera">ACameraManager_openCamera</a>) or the Java API ({@link android.hardware.camera2.CameraManager#openCamera}). </li> <li> Shared mode, use the NDK API (<a href="https://android.googlesource.com/platform/frameworks/av/+/refs/heads/main/camera/ndk/include/camera/NdkCameraManager.h#345">ACameraManager_openSharedCamera</a>) or the Java API ({@link android.hardware.camera2.CameraManager#openSharedCamera}). To enable camera sharing, provide a shared session configuration. </li> </ul> </li> <li> To configure streams, create a capture session with the relevant output surfaces. For example, from an {@link android.media.ImageReader} or {@link android.view.SurfaceView} with <a href="https://developer.android.com/ndk/reference/group/camera#acameradevice_createcapturesession"> ACameraDevice_createCaptureSession</a> (NDK) or {@link android.hardware.camera2.CameraDevice#createCaptureSession} (Java). Camera2 supports simultaneous <a href="https://developer.android.com/media/camera/camera2/multiple-camera-streams-simultaneously"> multiple streams</a>. Create multiple streams for purposes such as for preview, recording, and image processing. Streams serve as parallel pipelines, sequentially processing raw frames from the camera. </li> </ol>
   */
  android.hardware.automotive.evs.IEvsCamera openCamera(in String cameraId, in android.hardware.automotive.evs.Stream streamCfg);
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use {@link android.view.ViewManager#addView} on a specific {@link android.view.Display} instead.
   */
  android.hardware.automotive.evs.IEvsDisplay openDisplay(in int id);
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use <a href="https://source.android.com/docs/automotive/camera/acs/camera2-migration#ultrasonics-apis"> Ultrasonics VHAL properties</a> instead.
   */
  android.hardware.automotive.evs.IEvsUltrasonicsArray openUltrasonicsArray(in String ultrasonicsArrayId);
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use either the Camera2 NDK APIs or the Camera2 Java APIs instead. For the NDK: <a href="https://developer.android.com/ndk/reference/group/camera#acameramanager_registeravailabilitycallback">ACameraManager_registerAvailabilityCallback</a> with <a href="https://developer.android.com/ndk/reference/struct/a-camera-manager-availability-listener">ACameraManager_AvailabilityListener</a>. For Java: {@link android.hardware.camera2.CameraManager#registerAvailabilityCallback} with {@link android.hardware.camera2.CameraManager.AvailabilityCallback}.
   */
  void registerStatusCallback(in android.hardware.automotive.evs.IEvsEnumeratorStatusCallback callback);
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use {@link android.hardware.display.DisplayManager#getDisplay} and then {@link android.hardware.display.DisplayManager#getState} to retrieve the state for a specific display instead.
   */
  android.hardware.automotive.evs.DisplayState getDisplayStateById(in int id);
}
