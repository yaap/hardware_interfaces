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

import android.hardware.automotive.evs.BufferDesc;
import android.hardware.automotive.evs.CameraDesc;
import android.hardware.automotive.evs.CameraParam;
import android.hardware.automotive.evs.IEvsCameraStream;
import android.hardware.automotive.evs.IEvsDisplay;
import android.hardware.automotive.evs.ParameterRange;

/**
 * Represents a single camera and is the primary interface for capturing images.
 *
 * @deprecated EVS functionality and APIs are deprecated.
 *             Applications should use the standard Android <a
 *             href="https://developer.android.com/media/camera/camera2">Camera2 API
 *             (android.hardware.camera2)</a> for camera access and management. Use either the
 *             Camera2 NDK APIs (<a
 *             href="https://developer.android.com/ndk/reference/group/camera#acameramanager">ACameraManager</a>)
 *             or Camera2 Java APIs ({@link android.hardware.camera2.CameraManager}) instead.
 */
@VintfStability
interface IEvsCamera {
    /**
     * Returns frames that were delivered to the IEvsCameraStream.
     *
     * When done consuming a frame delivered to the IEvsCameraStream
     * interface, it must be returned to the IEvsCamera for reuse.
     * A small, finite number of buffers are available (possibly as small
     * as one), and if the supply is exhausted, no further frames may be
     * delivered until a buffer is returned.
     *
     * @param in buffer Buffers to be returned.
     *
     * @deprecated EVS functionality and APIs are deprecated. Use the Camera2 NDK API (<a
     *             href="https://developer.android.com/ndk/reference/group/media#aimage_delete">AImage_delete</a>)
     *             or the Camera2 Java API ({@link android.media.Image#close}) instead.
     */
    void doneWithFrame(in BufferDesc[] buffer);

    /**
     * Sets to be the primary client forcibly.
     *
     * The client, which owns the display, has a high priority and can take over
     * a primary client role from other clients without the display.
     *
     * @param in display IEvsDisplay handle.  If a given display is in either
     *                   NOT_VISIBLE, VISIBLE_ON_NEXT_FRAME, or VISIBLE state, the
     *                   calling client is considered as the high priority client
     *                   and therefore allowed to take over a primary client role from
     *                   existing primary client.
     * @throws EvsResult::INVALID_ARG if a given display handle is null or invalid states.
     *
     * @deprecated EVS functionality and APIs are deprecated.
     */
    void forcePrimaryClient(in IEvsDisplay display);

    /**
     * Returns the description of this camera.
     *
     * @return The description of this camera.  This must be the same value as
     *         reported by IEvsEnumerator::getCameraList().
     *
     * @deprecated EVS functionality and APIs are deprecated.
     *             Use the Camera2 NDK API (<a
     *             href="https://developer.android.com/ndk/reference/group/camera#acameramanager_getcameracharacteristics">ACameraManager_getCameraCharacteristics</a>)
     *             or the Camera2 Java API ({@link
     *             android.hardware.camera2.CameraManager#getCameraCharacteristics}) instead.
     */
    CameraDesc getCameraInfo();

    /**
     * Request driver specific information from the HAL implementation.
     *
     * The values allowed for opaqueIdentifier are driver specific,
     * but no value passed in may crash the driver.
     *
     * @param in opaqueIdentifier An unique identifier of the information to
     *                            request.
     * @return Requested information.  Zero-size vector is returned if the driver does
     *         not recognize a given identifier.
     * @throws EvsResult::INVALID_ARG for any unrecognized opaqueIdentifier.
     *
     * @deprecated EVS functionality and APIs are deprecated.
     *             Use either the Camera2 NDK APIs or the Camera2 Java APIs instead.
     *             For the NDK:
     *             Use vendor tags on <a
     *             href="https://developer.android.com/ndk/reference/group/camera#acapturerequest">ACaptureRequest</a>.
     *             For Java:
     *             Use vendor tags on {@link android.hardware.camera2.CaptureRequest}.
     */
    byte[] getExtendedInfo(in int opaqueIdentifier);

    /**
     * Retrieves values of given camera parameter.  The driver must report
     * EvsResult::INVALID_ARG if a request parameter is not supported.
     *
     * @param in id The identifier of camera parameter, CameraParam enum.
     * @return Values of requested camera parameter, the same number of values as
     *         backing camera devices.
     * @throws EvsResult::INVALID_ARG for any unrecognized parameter.
     *        EvsResult::UNDERLYING_SERVICE_ERROR for any other failures.
     *
     * @deprecated EVS functionality and APIs are deprecated.
     *             Use the Camera2 NDK API (<a
     *             href="https://developer.android.com/ndk/reference/group/camera#acameracapturesession_capturecallback_result">ACameraCaptureSession_captureCallback_result</a>)
     *             or the Camera2 Java API ({@link android.hardware.camera2.CaptureResult#get})
     *             instead.
     */
    int[] getIntParameter(in CameraParam id);

    /**
     * Requests a valid value range of a camera parameter
     *
     * @param in id The identifier of camera parameter, CameraParam enum.
     * @return ParameterRange of a requested CameraParam
     *
     * @deprecated EVS functionality and APIs are deprecated. Use the Camera2 NDK API (<a
     *             href="https://developer.android.com/ndk/reference/group/camera#acamerametadata_getconstentry">ACameraMetadata_getConstEntry</a>)
     *             or the Camera2 Java API ({@link
     *             android.hardware.camera2.CameraCharacteristics#get}) instead.
     */
    ParameterRange getIntParameterRange(in CameraParam id);

    /**
     * Retrieves a list of parameters this camera supports.
     *
     * @return A list of CameraParam that this camera supports.
     *
     * @deprecated EVS functionality and APIs are deprecated. Use the Camera2 NDK API (<a
     *             href="https://developer.android.com/ndk/reference/group/camera#acapturerequest_getalltags">ACaptureRequest_getAllTags</a>)
     *             or the Camera2 Java API ({@link
     *             android.hardware.camera2.CameraCharacteristics#getAvailableCaptureRequestKeys})
     *             instead.
     */
    CameraParam[] getParameterList();

    /**
     * Returns the description of the physical camera device that backs this
     * logical camera.
     *
     * If a requested device does not either exist or back this logical device,
     * this method returns a null camera descriptor.  And, if this is called on
     * a physical camera device, this method is the same as getCameraInfo()
     * method if a given device ID is matched.  Otherwise, this will return a
     * null camera descriptor.
     *
     * @param in deviceId Physical camera device identifier string.
     * @return The description of a member physical camera device.
     *         This must be the same value as reported by IEvsEnumerator::getCameraList().
     *
     * @deprecated EVS functionality and APIs are deprecated.
     *             For logical cameras, call {@link
     *             android.hardware.camera2.CameraCharacteristics#getPhysicalCameraIds} to retrieve
     *             the camera IDs of the physical cameras. Then, use these IDs in the following
     *             Camera2 API calls to access the physical camera information:
     *             For the NDK:
     *             <a
     *             href="https://developer.android.com/ndk/reference/group/camera#acameramanager_getcameracharacteristics">ACameraManager_getCameraCharacteristics</a>.
     *             For Java:
     *             {@link android.hardware.camera2.CameraManager#getCameraCharacteristics}.
     */
    CameraDesc getPhysicalCameraInfo(in String deviceId);

    /**
     * Import external buffers to capture frames
     *
     * This API must be called with a physical camera device identifier.
     *
     * @param in buffers A list of buffers allocated by the caller.  EvsCamera
     *                   will use these buffers to capture frames, in addition to
     *                   other buffers already in its buffer pool.
     * @return The amount of buffer pool size changes after importing given buffers.
     *
     * @deprecated EVS functionality and APIs are deprecated.
     *             In Camera2, buffer management is handled by Surfaces. To pre-allocate buffers,
     *             use the Camera 2 NDK API (<a
     *             href="https://developer.android.com/ndk/reference/group/camera#acameracapturesession_preparewindow">ACameraCaptureSession_prepareWindow</a>)
     *             or the Camera2 Java API ({@link
     *             android.hardware.camera2.CameraCaptureSession#prepare}) instead.
     */
    int importExternalBuffers(in BufferDesc[] buffers);

    /**
     * Requests to pause EVS camera stream events.
     *
     * Like stopVideoStream(), events may continue to arrive for some time
     * after this call returns. Delivered frame buffers must be returned.
     *
     * @deprecated EVS functionality and APIs are deprecated.
     *             Use the Camera2 NDK API (<a
     *             href="https://developer.android.com/ndk/reference/group/camera#acameracapturesession_stoprepeating">ACameraCaptureSession_stopRepeating</a>)
     *             or the Camera2 Java API ({@link
     *             android.hardware.camera2.CameraCaptureSession#stopRepeating}) instead.
     */
    void pauseVideoStream();

    /**
     * Requests to resume EVS camera stream.
     *
     * @deprecated EVS functionality and APIs are deprecated.
     *             Use the Camera2 NDK API (<a
     *             href="https://developer.android.com/ndk/reference/group/camera#acameracapturesession_setrepeatingrequestv2">ACameraCaptureSession_setRepeatingRequestV2</a>)
     *             or the Camera2 Java API ({@link
     *             android.hardware.camera2.CameraCaptureSession#setSingleRepeatingRequest})
     *             instead.
     */
    void resumeVideoStream();

    /**
     * Send a driver specific value to the HAL implementation.
     *
     * This extension is provided to facilitate car specific
     * extensions, but no HAL implementation may require this call
     * in order to function in a default state.
     * INVALID_ARG is returned if the opaqueValue is not meaningful to
     * the driver implementation.
     *
     * @param in opaqueIdentifier An unique identifier of the information to
     *                            program.
     *        in opaqueValue A value to program.
     * @throws EvsResult::INVALID_ARG if this call fails to set a parameter.
     *
     * @deprecated EVS functionality and APIs are deprecated.
     *             Use either the Camera2 NDK APIs or the Camera2 Java APIs instead, with vendor
     *             tags to implement custom data.
     *             For the NDK:
     *             Use vendor tags on <a
     *             href="https://developer.android.com/ndk/reference/group/camera#acapturerequest">ACaptureRequest</a>.
     *             For Java:
     *             Use vendor tags on {@link android.hardware.camera2.CaptureRequest.Builder}.
     */
    void setExtendedInfo(in int opaqueIdentifier, in byte[] opaqueValue);

    /**
     * Requests to set a camera parameter.
     *
     * Only a request from the primary client will be processed successfully.
     * When this method is called on a logical camera device, it will be forwarded
     * to each physical device and, if it fails to program any physical device,
     * it will return an error code with the same number of effective values as
     * the number of backing camera devices.
     *
     * @param in id The identifier of camera parameter, CameraParam enum.
     * @param in value A desired parameter value.
     * @return Programmed parameter values.  This may differ from what the client
     *         gives if, for example, the driver does not support a target parameter.
     * @throws EvsResult::INVALID_ARG if either the request is not made by the primary
     *        client, or a requested parameter is not supported.
     *        EvsResult::UNDERLYING_SERVICE_ERROR if it fails to program a value by any
     *        other reason.
     *
     * @deprecated EVS functionality and APIs are deprecated.
     *             Use either the Camera2 NDK APIs or the Camera2 Java APIs instead.
     *             For the NDK:
     *             Set the parameter based on the type of data using
     *             <a
     *             href="https://developer.android.com/ndk/reference/group/camera#acapturerequest_setentry_float">ACaptureRequest_setEntry_float</a>,
     *             <a
     *             href="https://developer.android.com/ndk/reference/group/camera#acapturerequest_setentry_double">ACaptureRequest_setEntry_double</a>,
     *             <a
     *             href="https://developer.android.com/ndk/reference/group/camera#acapturerequest_setentry_i32">ACaptureRequest_setEntry_i32</a>,
     *             <a
     *             href="https://developer.android.com/ndk/reference/group/camera#acapturerequest_setentry_i64">ACaptureRequest_setEntry_i64</a>,
     *             <a
     *             href="https://developer.android.com/ndk/reference/group/camera#acapturerequest_setentry_rational">ACaptureRequest_setEntry_rational</a>,
     *             <a
     *             href="https://developer.android.com/ndk/reference/group/camera#acapturerequest_setentry_u8">ACaptureRequest_setEntry_u8</a>,
     *             or similar.
     *             For Java:
     *             {@link android.hardware.camera2.CaptureRequest.Builder#set}.
     */
    int[] setIntParameter(in CameraParam id, in int value);

    /**
     * Requests to be the primary client.
     *
     * When multiple clients subscribe to a single camera hardware and one of
     * them adjusts a camera parameter such as the contrast, it may disturb
     * other clients' operations.  Therefore, the client must call this method
     * to be a primary client.  Once it becomes a primary client, it will be able to
     * change camera parameters until either it dies or explicitly gives up the
     * role.
     *
     * @throws EvsResult::OWNERSHIP_LOST if there is already the primary client.
     *
     * @deprecated EVS functionality and APIs are deprecated.
     */
    void setPrimaryClient();

    /**
     * Specifies the depth of the buffer chain the camera is asked to support.
     *
     * Up to this many frames may be held concurrently by the client of IEvsCamera.
     * If this many frames have been delivered to the receiver without being returned
     * by doneWithFrame, the stream must skip frames until a buffer is returned for reuse.
     * It is legal for this call to come at any time, even while streams are already running,
     * in which case buffers should be added or removed from the chain as appropriate.
     * If no call is made to this entry point, the IEvsCamera must support at least one
     * frame by default. More is acceptable.
     *
     * @param in bufferCount Number of buffers the client of IEvsCamera may hold concurrently.
     * @throws EvsResult::BUFFER_NOT_AVAILABLE if the client cannot increase the max frames.
     *        EvsResult::INVALID_ARG if the client cannot decrease the max frames.
     *        EvsResult::OWNERSHIP_LOST if we lost an ownership of a target camera.
     *
     * @deprecated EVS functionality and APIs are deprecated.
     *             Use either the Camera2 NDK APIs or the Camera2 Java APIs instead to set the
     *             maximum number of images the user will want to access simultaneously.
     *             For the NDK:
     *             Set in <a
     *             href="https://developer.android.com/ndk/reference/group/media#aimagereader_new">AImageReader_new</a>
     *             or <a
     *             href="https://developer.android.com/ndk/reference/group/media#aimagereader_newwithusage">AImageReader_newWithUsage</a>.
     *             For Java:
     *             Set in {@link android.media.ImageReader#newInstance}.
     */
    void setMaxFramesInFlight(in int bufferCount);

    /**
     * Request to start EVS camera stream from this camera.
     *
     * The IEvsCameraStream must begin receiving calls with various events
     * including new image frame ready until stopVideoStream() is called.
     *
     * @param in receiver IEvsCameraStream implementation.
     * @throws EvsResult::OWNERSHIP_LOST if we lost an ownership of a target camera.
     *        EvsResult::STREAM_ALREADY_RUNNING if a video stream has been started already.
     *        EvsResult::BUFFER_NOT_AVAILABLE if it fails to secure a minimum number of
     *        buffers to run a video stream.
     *        EvsResult::UNDERLYING_SERVICE_ERROR for all other failures.
     *
     * @deprecated EVS functionality and APIs are deprecated.
     *             Use the Camera2 NDK API (<a
     *             href="https://developer.android.com/ndk/reference/group/camera#acameracapturesession_setrepeatingrequestv2">ACameraCaptureSession_setRepeatingRequestV2</a>)
     *             or the Camera2 Java API ({@link
     *             android.hardware.camera2.CameraCaptureSession#setSingleRepeatingRequest})
     *             instead.
     */
    void startVideoStream(in IEvsCameraStream receiver);

    /**
     * Stop the delivery of EVS camera frames.
     *
     * Because delivery is asynchronous, frames may continue to arrive for
     * some time after this call returns. Each must be returned until the
     * closure of the stream is signaled to the IEvsCameraStream.
     * This function cannot fail and is simply ignored if the stream isn't running.
     *
     * @deprecated EVS functionality and APIs are deprecated.
     *             Use the Camera2 NDK API (<a
     *             href="https://developer.android.com/ndk/reference/group/camera#acameracapturesession_stoprepeating">ACameraCaptureSession_stopRepeating</a>)
     *             or the Camera2 Java API ({@link
     *             android.hardware.camera2.CameraCaptureSession#stopRepeating}) instead.
     */
    void stopVideoStream();

    /**
     * Retires from the primary client role.
     *
     * @throws EvsResult::INVALID_ARG if the caller client is not a primary client.
     *
     * @deprecated EVS functionality and APIs are deprecated.
     */
    void unsetPrimaryClient();
}
