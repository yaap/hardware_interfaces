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
interface IEvsCamera {
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use the Camera2 NDK API (<a href="https://developer.android.com/ndk/reference/group/media#aimage_delete">AImage_delete</a>) or the Camera2 Java API ({@link android.media.Image#close}) instead.
   */
  void doneWithFrame(in android.hardware.automotive.evs.BufferDesc[] buffer);
  /**
   * @deprecated EVS functionality and APIs are deprecated.
   */
  void forcePrimaryClient(in android.hardware.automotive.evs.IEvsDisplay display);
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use the Camera2 NDK API (<a href="https://developer.android.com/ndk/reference/group/camera#acameramanager_getcameracharacteristics">ACameraManager_getCameraCharacteristics</a>) or the Camera2 Java API ({@link android.hardware.camera2.CameraManager#getCameraCharacteristics}) instead.
   */
  android.hardware.automotive.evs.CameraDesc getCameraInfo();
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use either the Camera2 NDK APIs or the Camera2 Java APIs instead. For the NDK: Use vendor tags on <a href="https://developer.android.com/ndk/reference/group/camera#acapturerequest">ACaptureRequest</a>. For Java: Use vendor tags on {@link android.hardware.camera2.CaptureRequest}.
   */
  byte[] getExtendedInfo(in int opaqueIdentifier);
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use the Camera2 NDK API (<a href="https://developer.android.com/ndk/reference/group/camera#acameracapturesession_capturecallback_result">ACameraCaptureSession_captureCallback_result</a>) or the Camera2 Java API ({@link android.hardware.camera2.CaptureResult#get}) instead.
   */
  int[] getIntParameter(in android.hardware.automotive.evs.CameraParam id);
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use the Camera2 NDK API (<a href="https://developer.android.com/ndk/reference/group/camera#acamerametadata_getconstentry">ACameraMetadata_getConstEntry</a>) or the Camera2 Java API ({@link android.hardware.camera2.CameraCharacteristics#get}) instead.
   */
  android.hardware.automotive.evs.ParameterRange getIntParameterRange(in android.hardware.automotive.evs.CameraParam id);
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use the Camera2 NDK API (<a href="https://developer.android.com/ndk/reference/group/camera#acapturerequest_getalltags">ACaptureRequest_getAllTags</a>) or the Camera2 Java API ({@link android.hardware.camera2.CameraCharacteristics#getAvailableCaptureRequestKeys}) instead.
   */
  android.hardware.automotive.evs.CameraParam[] getParameterList();
  /**
   * @deprecated EVS functionality and APIs are deprecated. For logical cameras, call {@link android.hardware.camera2.CameraCharacteristics#getPhysicalCameraIds} to retrieve the camera IDs of the physical cameras. Then, use these IDs in the following Camera2 API calls to access the physical camera information: For the NDK: <a href="https://developer.android.com/ndk/reference/group/camera#acameramanager_getcameracharacteristics">ACameraManager_getCameraCharacteristics</a>. For Java: {@link android.hardware.camera2.CameraManager#getCameraCharacteristics}.
   */
  android.hardware.automotive.evs.CameraDesc getPhysicalCameraInfo(in String deviceId);
  /**
   * @deprecated EVS functionality and APIs are deprecated. In Camera2, buffer management is handled by Surfaces. To pre-allocate buffers, use the Camera 2 NDK API (<a href="https://developer.android.com/ndk/reference/group/camera#acameracapturesession_preparewindow">ACameraCaptureSession_prepareWindow</a>) or the Camera2 Java API ({@link android.hardware.camera2.CameraCaptureSession#prepare}) instead.
   */
  int importExternalBuffers(in android.hardware.automotive.evs.BufferDesc[] buffers);
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use the Camera2 NDK API (<a href="https://developer.android.com/ndk/reference/group/camera#acameracapturesession_stoprepeating">ACameraCaptureSession_stopRepeating</a>) or the Camera2 Java API ({@link android.hardware.camera2.CameraCaptureSession#stopRepeating}) instead.
   */
  void pauseVideoStream();
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use the Camera2 NDK API (<a href="https://developer.android.com/ndk/reference/group/camera#acameracapturesession_setrepeatingrequestv2">ACameraCaptureSession_setRepeatingRequestV2</a>) or the Camera2 Java API ({@link android.hardware.camera2.CameraCaptureSession#setSingleRepeatingRequest}) instead.
   */
  void resumeVideoStream();
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use either the Camera2 NDK APIs or the Camera2 Java APIs instead, with vendor tags to implement custom data. For the NDK: Use vendor tags on <a href="https://developer.android.com/ndk/reference/group/camera#acapturerequest">ACaptureRequest</a>. For Java: Use vendor tags on {@link android.hardware.camera2.CaptureRequest.Builder}.
   */
  void setExtendedInfo(in int opaqueIdentifier, in byte[] opaqueValue);
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use either the Camera2 NDK APIs or the Camera2 Java APIs instead. For the NDK: Set the parameter based on the type of data using <a href="https://developer.android.com/ndk/reference/group/camera#acapturerequest_setentry_float">ACaptureRequest_setEntry_float</a>, <a href="https://developer.android.com/ndk/reference/group/camera#acapturerequest_setentry_double">ACaptureRequest_setEntry_double</a>, <a href="https://developer.android.com/ndk/reference/group/camera#acapturerequest_setentry_i32">ACaptureRequest_setEntry_i32</a>, <a href="https://developer.android.com/ndk/reference/group/camera#acapturerequest_setentry_i64">ACaptureRequest_setEntry_i64</a>, <a href="https://developer.android.com/ndk/reference/group/camera#acapturerequest_setentry_rational">ACaptureRequest_setEntry_rational</a>, <a href="https://developer.android.com/ndk/reference/group/camera#acapturerequest_setentry_u8">ACaptureRequest_setEntry_u8</a>, or similar. For Java: {@link android.hardware.camera2.CaptureRequest.Builder#set}.
   */
  int[] setIntParameter(in android.hardware.automotive.evs.CameraParam id, in int value);
  /**
   * @deprecated EVS functionality and APIs are deprecated.
   */
  void setPrimaryClient();
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use either the Camera2 NDK APIs or the Camera2 Java APIs instead to set the maximum number of images the user will want to access simultaneously. For the NDK: Set in <a href="https://developer.android.com/ndk/reference/group/media#aimagereader_new">AImageReader_new</a> or <a href="https://developer.android.com/ndk/reference/group/media#aimagereader_newwithusage">AImageReader_newWithUsage</a>. For Java: Set in {@link android.media.ImageReader#newInstance}.
   */
  void setMaxFramesInFlight(in int bufferCount);
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use the Camera2 NDK API (<a href="https://developer.android.com/ndk/reference/group/camera#acameracapturesession_setrepeatingrequestv2">ACameraCaptureSession_setRepeatingRequestV2</a>) or the Camera2 Java API ({@link android.hardware.camera2.CameraCaptureSession#setSingleRepeatingRequest}) instead.
   */
  void startVideoStream(in android.hardware.automotive.evs.IEvsCameraStream receiver);
  /**
   * @deprecated EVS functionality and APIs are deprecated. Use the Camera2 NDK API (<a href="https://developer.android.com/ndk/reference/group/camera#acameracapturesession_stoprepeating">ACameraCaptureSession_stopRepeating</a>) or the Camera2 Java API ({@link android.hardware.camera2.CameraCaptureSession#stopRepeating}) instead.
   */
  void stopVideoStream();
  /**
   * @deprecated EVS functionality and APIs are deprecated.
   */
  void unsetPrimaryClient();
}
