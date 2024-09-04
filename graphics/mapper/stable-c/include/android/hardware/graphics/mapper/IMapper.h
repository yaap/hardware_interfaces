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

/**
 *  IMapper Stable-C HAL interface
 *
 *  This file represents the sphal interface between libui & the IMapper HAL implementation.
 *  A vendor implementation of this interface is retrieved by looking up the vendor imapper
 *  implementation library via the IAllocator AIDL interface.
 *
 *  This interface is not intended for general use.
 */

#pragma once

#include <stdint.h>
#include <sys/cdefs.h>

#include <android/rect.h>
#include <cutils/native_handle.h>

__BEGIN_DECLS

/**
 * AIMapper versioning
 *
 * IMapper versions 0-1 are pre-treble
 * IMapper versions 2-4 are HIDL
 * C-style AIMapper API starts at 5
 */
enum AIMapper_Version : uint32_t {
    AIMAPPER_VERSION_5 = 5,
};

/**
 * Possible AIMapper errors
 * Values are the same as IMapper 4.0's Error type for simplicity
 */
enum AIMapper_Error : int32_t {
    /**
     * No error.
     */
    AIMAPPER_ERROR_NONE = 0,
    /**
     * Invalid BufferDescriptor.
     */
    AIMAPPER_ERROR_BAD_DESCRIPTOR = 1,
    /**
     * Invalid buffer handle.
     */
    AIMAPPER_ERROR_BAD_BUFFER = 2,
    /**
     * Invalid HardwareBufferDescription.
     */
    AIMAPPER_ERROR_BAD_VALUE = 3,
    /**
     * Resource unavailable.
     */
    AIMAPPER_ERROR_NO_RESOURCES = 5,
    /**
     * Permanent failure.
     */
    AIMAPPER_ERROR_UNSUPPORTED = 7,
};

/**
 * MetadataType represents the different types of buffer metadata that could be
 * associated with a buffer. It is used by IMapper to help get and set buffer metadata
 * on the buffer's native handle.
 *
 * Standard buffer metadata will have the name field set to
 * "android.hardware.graphics.common.StandardMetadataType" and will contain values
 * from StandardMetadataType.aidl.
 *
 * Vendor-provided metadata should be prefixed with a "vendor.mycompanyname.*" namespace. It is
 * recommended that the metadata follows the pattern of StandardMetadaType.aidl. That is, an
 * aidl-defined enum with @VintfStability on it and the naming then matching that type such
 * as "vendor.mycompanyname.graphics.common.MetadataType" with the value field then set to the
 * aidl's enum value.
 *
 * Each company should create their own enum & namespace. The name
 * field prevents values from different companies from colliding.
 */
typedef struct AIMapper_MetadataType {
    const char* _Nonnull name;
    int64_t value;
} AIMapper_MetadataType;

typedef struct AIMapper_MetadataTypeDescription {
    /**
     * The `name` of the metadataType must be valid for the lifetime of the process
     */
    AIMapper_MetadataType metadataType;
    /**
     * description should contain a string representation of the MetadataType.
     *
     * For example: "MyExampleMetadataType is a 64-bit timestamp in nanoseconds
     * that indicates when a buffer is decoded. It is set by the media HAL after
     * a buffer is decoded. It is used by the display HAL for hardware
     * synchronization".
     *
     * This field is required for any non-StandardMetadataTypes. For StandardMetadataTypes this
     * field may be null. The lifetime of this pointer must be valid for the duration of the
     * process (that is, a static const char*).
     */
    const char* _Nullable description;
    /**
     * isGettable represents if the MetadataType can be get.
     */
    bool isGettable;
    /**
     * isSettable represents if the MetadataType can be set.
     */
    bool isSettable;

    /** Reserved for future use; must be zero-initialized currently */
    uint8_t reserved[32];
} AIMapper_MetadataTypeDescription;

/**
 * Callback that is passed to dumpBuffer.
 *
 * @param context The caller-provided void* that was passed to dumpBuffer.
 * @param metadataType The type of the metadata passed to the callback
 * @param value A pointer to the value of the metadata. The lifetime of this pointer is only
 *              valid for the duration of the call
 * @param valueSize The size of the value buffer.
 */
typedef void (*AIMapper_DumpBufferCallback)(void* _Null_unspecified context,
                                            AIMapper_MetadataType metadataType,
                                            const void* _Nonnull value, size_t valueSize);

/**
 * Callback that is passed to dumpAllBuffers.
 *
 * Indicates that a buffer is about to be dumped. Will be followed by N calls to
 * AIMapper_DumpBufferCallback for all the metadata for this buffer.
 *
 * @param context The caller-provided void* that was passed to dumpAllBuffers.
 */
typedef void (*AIMapper_BeginDumpBufferCallback)(void* _Null_unspecified context);

/**
 * Implementation of AIMAPPER_VERSION_5
 * All functions must not be null & must provide a valid implementation.
 */
typedef struct AIMapperV5 {
    /**
     * Imports a raw buffer handle to create an imported buffer handle for use
     * with the rest of the mapper or with other in-process libraries.
     *
     * A buffer handle is considered raw when it is cloned (e.g., with
     * `native_handle_clone()`) from another buffer handle locally, or when it
     * is received from another HAL server/client or another process. A raw
     * buffer handle must not be used to access the underlying graphic
     * buffer. It must be imported to create an imported handle first.
     *
     * This function must at least validate the raw handle before creating the
     * imported handle. It must also support importing the same raw handle
     * multiple times to create multiple imported handles. The imported handle
     * must be considered valid everywhere in the process, including in
     * another instance of the mapper.
     *
     * Because of passthrough HALs, a raw buffer handle received from a HAL
     * may actually have been imported in the process. importBuffer() must treat
     * such a handle as if it is raw and must not return `BAD_BUFFER`. The
     * returned handle is independent from the input handle as usual, and
     * freeBuffer() must be called on it when it is no longer needed.
     *
     * @param handle Raw buffer handle to import.
     * @param outBufferHandle The resulting imported buffer handle.
     * @return Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the raw handle is invalid.
     *     - `NO_RESOURCES` if the raw handle cannot be imported due to
     *       unavailability of resources.
     */
    AIMapper_Error (*_Nonnull importBuffer)(const native_handle_t* _Nonnull handle,
                                            buffer_handle_t _Nullable* _Nonnull outBufferHandle);

    /**
     * Frees a buffer handle. Buffer handles returned by importBuffer() must be
     * freed with this function when no longer needed.
     *
     * This function must free up all resources allocated by importBuffer() for
     * the imported handle. For example, if the imported handle was created
     * with `native_handle_create()`, this function must call
     * `native_handle_close()` and `native_handle_delete()`.
     *
     * @param buffer Imported buffer handle.
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the buffer is invalid.
     */
    AIMapper_Error (*_Nonnull freeBuffer)(buffer_handle_t _Nonnull buffer);

    /**
     * Calculates the transport size of a buffer. An imported buffer handle is a
     * raw buffer handle with the process-local runtime data appended. This
     * function, for example, allows a caller to omit the process-local runtime
     * data at the tail when serializing the imported buffer handle.
     *
     * Note that a client might or might not omit the process-local runtime data
     * when sending an imported buffer handle. The mapper must support both
     * cases on the receiving end.
     *
     * @param buffer Buffer to get the transport size from.
     * @param outNumFds The number of file descriptors needed for transport.
     * @param outNumInts The number of integers needed for transport.
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the buffer is invalid.
     */
    AIMapper_Error (*_Nonnull getTransportSize)(buffer_handle_t _Nonnull buffer,
                                                uint32_t* _Nonnull outNumFds,
                                                uint32_t* _Nonnull outNumInts);

    /**
     * Locks the given buffer for the specified CPU usage.
     *
     * Locking the same buffer simultaneously from multiple threads is
     * permitted, but if any of the threads attempt to lock the buffer for
     * writing, the behavior is undefined, except that it must not cause
     * process termination or block the client indefinitely. Leaving the
     * buffer content in an indeterminate state or returning an error are both
     * acceptable.
     *
     * 1D buffers (width = size in bytes, height = 1, pixel_format = BLOB) must
     * "lock in place" and behave similar to shared memory. That is, multiple threads or processes
     * may lock the buffer for reading & writing and the results must follow the device's memory
     * model.
     *
     * The client must not modify the content of the buffer outside of
     * @p accessRegion, and the device need not guarantee that content outside
     * of @p accessRegion is valid for reading. The result of reading or writing
     * outside of @p accessRegion is undefined, except that it must not cause
     * process termination.
     *
     * An accessRegion of all-zeros means the entire buffer. That is, it is
     * equivalent to '(0,0)-(buffer width, buffer height)'.
     *
     * This function can lock both single-planar and multi-planar formats. The caller
     * should use get() to get information about the buffer they are locking.
     * get() can be used to get information about the planes, offsets, stride,
     * etc.
     *
     * This function must also work on buffers with
     * `AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_*` if supported by the device, as well
     * as with any other formats requested by multimedia codecs when they are
     * configured with a flexible-YUV-compatible color format.
     *
     * On success, @p data must be filled with a pointer to the locked buffer
     * memory. This address will represent the top-left corner of the entire
     * buffer, even if @p accessRegion does not begin at the top-left corner.
     *
     * The locked buffer must adhere to the format requested at allocation time
     * in the BufferDescriptorInfo.
     *
     * @param buffer Buffer to lock.
     * @param cpuUsage CPU usage flags to request. See BufferUsage.aidl for possible values.
     * @param accessRegion Portion of the buffer that the client intends to
     *     access.
     * @param acquireFence Handle containing a file descriptor referring to a
     *     sync fence object, which will be signaled when it is safe for the
     *     mapper to lock the buffer. @p acquireFence may be an empty fence (-1) if
     *     it is already safe to lock. Ownership is passed to the callee and it is the
     *     implementations responsibility to ensure it is closed even when an error
     *     occurs.
     * @param outData CPU-accessible pointer to the buffer data.
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the buffer is invalid or is incompatible with this
     *       function.
     *     - `BAD_VALUE` if @p cpuUsage is 0, contains non-CPU usage flags, or
     *       is incompatible with the buffer. Also if the @p accessRegion is
     *       outside the bounds of the buffer or the accessRegion is invalid.
     *     - `NO_RESOURCES` if the buffer cannot be locked at this time. Note
     *       that locking may succeed at a later time.
     * @return data CPU-accessible pointer to the buffer data.
     */
    AIMapper_Error (*_Nonnull lock)(buffer_handle_t _Nonnull buffer, uint64_t cpuUsage,
                                    ARect accessRegion, int acquireFence,
                                    void* _Nullable* _Nonnull outData);

    /**
     * Unlocks a buffer to indicate all CPU accesses to the buffer have
     * completed.
     *
     * @param buffer Buffer to unlock.
     * @param releaseFence Handle containing a file descriptor referring to a
     *     sync fence object. The sync fence object will be signaled when the
     *     mapper has completed any pending work. @p releaseFence may be an
     *     empty fence (-1).
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the buffer is invalid or not locked.
     */
    AIMapper_Error (*_Nonnull unlock)(buffer_handle_t _Nonnull buffer, int* _Nonnull releaseFence);

    /**
     * Flushes the contents of a locked buffer.
     *
     * This function flushes the CPUs caches for the range of all the buffer's
     * planes and metadata. This should behave similarly to unlock() except the
     * buffer should remain mapped to the CPU.
     *
     * The client is still responsible for calling unlock() when it is done
     * with all CPU accesses to the buffer.
     *
     * If non-CPU blocks are simultaneously writing the buffer, the locked
     * copy should still be flushed but what happens is undefined except that
     * it should not cause any crashes.
     *
     * @param buffer Buffer to flush.
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the buffer is invalid or not locked.
     */
    AIMapper_Error (*_Nonnull flushLockedBuffer)(buffer_handle_t _Nonnull buffer);

    /**
     * Rereads the contents of a locked buffer.
     *
     * This should fetch the most recent copy of the locked buffer.
     *
     * It may reread locked copies of the buffer in other processes.
     *
     * The client is still responsible for calling unlock() when it is done
     * with all CPU accesses to the buffer.
     *
     * @param buffer Buffer to reread.
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the buffer is invalid or not locked.
     *     - `NO_RESOURCES` if the buffer cannot be reread at this time. Note
     *       that rereading may succeed at a later time.
     */
    AIMapper_Error (*_Nonnull rereadLockedBuffer)(buffer_handle_t _Nonnull buffer);

    /**
     * Description for get(...), set(...) and getFromBufferDescriptorInfo(...)
     *
     * ------------ Overview -----------------------------------
     * Gralloc 4 adds support for getting and setting buffer metadata on a buffer.
     *
     * To get buffer metadata, the client passes in a buffer handle and a token that
     * represents the type of buffer metadata they would like to get. IMapper returns
     * a byte stream that contains the buffer metadata. To set the buffer metadata, the
     * client passes in a buffer handle and a token that represents the type of buffer
     * metadata they would like to set and a byte stream that contains the buffer metadata
     * they are setting.
     *
     * Buffer metadata is global for a buffer. When the metadata is set on the buffer
     * in a process, the updated metadata should be available to all other processes.
     * Please see "Storing and Propagating Metadata" below for more details.
     *
     * The getter and setter functions have been optimized for easy vendor extension.
     * They do not require a formal extension to add support for getting and setting
     * vendor defined buffer metadata. See "Buffer Metadata Token" and
     * "Buffer Metadata Stream" below for more details.
     *
     * ------------ Storing and Propagating Metadata -----------
     * Buffer metadata must be global. Any changes to the metadata must be propagated
     * to all other processes immediately. Vendors may chose how they would like support
     * this functionality.
     *
     * We recommend supporting this functionality by allocating an extra page of shared
     * memory and storing it in the buffer's native_handle_t. The buffer metadata can
     * be stored in the extra page of shared memory. Set operations are automatically
     * propagated to all other processes.
     *
     * ------------ Buffer Metadata Synchronization ------------
     * There are no explicit buffer metadata synchronization primitives. Many devices
     * before gralloc 4 already support getting and setting of global buffer metadata
     * with no explicit synchronization primitives. Adding synchronization primitives
     * would just add unnecessary complexity.
     *
     * The general rule is if a process has permission to write to a buffer, they
     * have permission to write to the buffer's writable metadata. If a process has permission
     * to read from a buffer, they have permission to read the buffer's metadata.
     *
     * There is one exception to this rule. Fences CANNOT be used to protect a buffer's
     * metadata. A process should finish writing to a buffer's metadata before
     * sending the buffer to another process that will read or write to the buffer.
     * This exception is needed because sometimes userspace needs to read the
     * buffer's metadata before the buffer's contents are ready.
     *
     * As a simple example: an app renders to a buffer and then displays the buffer.
     * In this example when the app renders to the buffer, both the buffer and its
     * metadata need to be updated. The app's process queues up its work on the GPU
     * and gets back an acquire fence. The app's process must update the buffer's
     * metadata before enqueuing the buffer to SurfaceFlinger. The app process CANNOT
     * update the buffer's metadata after enqueuing the buffer. When HardwareComposer
     * receives the buffer, it is immediately safe to read the buffer's metadata
     * and use it to program the display driver. To read the buffer's contents,
     * display driver must still wait on the acquire fence.
     *
     * ------------ Buffer Metadata Token ----------------------
     * In order to allow arbitrary vendor defined metadata, the token used to access
     * metadata is defined defined as a struct that has a string representing
     * the enum type and an int that represents the enum value. The string protects
     * different enum values from colliding.
     *
     * The token struct (MetadataType) is defined as a C struct since it
     * is passed into a C function. The standard buffer metadata types are NOT
     * defined as a C enum but instead as an AIDL enum to allow for broader usage across
     * other HALs and libraries. By putting the enum in the
     * stable AIDL (hardware/interfaces/graphics/common/aidl/android/hardware/
     * graphics/common/StandardMetadataType.aidl), vendors will be able to optionally
     * choose to support future standard buffer metadata types without upgrading
     * IMapper versions. For more information see the description of "struct MetadataType".
     *
     * ------------ Buffer Metadata Stream ---------------------
     * The buffer metadata is get and set as a void* buffer. By getting
     * and setting buffer metadata as a generic buffer, vendors can use the standard
     * getters and setter functions defined here. Vendors do NOT need to add their own
     * getters and setter functions for each new type of buffer metadata.
     *
     * Converting buffer metadata into a byte stream can be non-trivial. For the standard
     * buffer metadata types defined in StandardMetadataType.aidl, there are also
     * support functions that will encode the buffer metadata into a byte stream
     * and decode the buffer metadata from a byte stream. We STRONGLY recommend using
     * these support functions. The framework will use them when getting and setting
     * metadata. The support functions are defined in
     * frameworks/native/libs/gralloc/types/include/gralloctypes/Gralloc4.h.
     */

    /**
     * Gets the buffer metadata for a given MetadataType.
     *
     * Buffer metadata can be changed after allocation so clients should avoid "caching"
     * the buffer metadata. For example, if the video resolution changes and the buffers
     * are not reallocated, several buffer metadata values may change without warning.
     * Clients should not expect the values to be constant. They should requery them every
     * frame. The only exception is buffer metadata that is determined at allocation
     * time. For StandardMetadataType values, only BUFFER_ID, NAME, WIDTH,
     * HEIGHT, LAYER_COUNT, PIXEL_FORMAT_REQUESTED and USAGE are safe to cache because
     * they are determined at allocation time.
     *
     * @param buffer Buffer containing desired metadata
     * @param metadataType MetadataType for the metadata value being queried
     * @param destBuffer Pointer to a buffer in which to store the result of the get() call; if
     * null, the computed output size or error must still be returned.
     * @param destBufferSize How large the destBuffer buffer is. If destBuffer is null this must be
     * 0.
     * @return The number of bytes written to `destBuffer` or which would have been written
     *         if `destBufferSize` was large enough.
     *         A negative value indicates an error, which may be
     *         - `BAD_BUFFER` if the raw handle is invalid.
     *         - `UNSUPPORTED` when metadataType is unknown/unsupported.
     *            IMapper must support getting all StandardMetadataType.aidl values defined
     *            at the time the device first launches.
     */
    int32_t (*_Nonnull getMetadata)(buffer_handle_t _Nonnull buffer,
                                    AIMapper_MetadataType metadataType, void* _Nullable destBuffer,
                                    size_t destBufferSize);

    /**
     * Gets the buffer metadata for a StandardMetadataType.
     *
     * This is equivalent to `getMetadata` when passed an AIMapper_MetadataType with name
     * set to "android.hardware.graphics.common.StandardMetadataType"
     *
     * Buffer metadata can be changed after allocation so clients should avoid "caching"
     * the buffer metadata. For example, if the video resolution changes and the buffers
     * are not reallocated, several buffer metadata values may change without warning.
     * Clients should not expect the values to be constant. They should requery them every
     * frame. The only exception is buffer metadata that is determined at allocation
     * time. For StandardMetadataType values, only BUFFER_ID, NAME, WIDTH,
     * HEIGHT, LAYER_COUNT, PIXEL_FORMAT_REQUESTED and USAGE are safe to cache because
     * they are determined at allocation time.
     *
     * @param buffer Buffer containing desired metadata
     * @param standardMetadataType StandardMetadataType for the metadata value being queried
     * @param destBuffer Pointer to a buffer in which to store the result of the get() call; if
     * null, the computed output size or error must still be returned.
     * @param destBufferSize How large the destBuffer buffer is. If destBuffer is null this must be
     * 0.
     * @return The number of bytes written to `destBuffer` or which would have been written
     *         if `destBufferSize` was large enough.
     *         A negative value indicates an error, which may be
     *         - `BAD_BUFFER` if the raw handle is invalid.
     *         - `UNSUPPORTED` when metadataType is unknown/unsupported.
     *            IMapper must support getting all StandardMetadataType.aidl values defined
     *            at the time the device first launches.
     */
    int32_t (*_Nonnull getStandardMetadata)(buffer_handle_t _Nonnull buffer,
                                            int64_t standardMetadataType,
                                            void* _Nullable destBuffer, size_t destBufferSize);

    /**
     * Sets the global value for a given MetadataType.
     *
     * Metadata fields are not required to be settable. This function can
     * return Error::UNSUPPORTED whenever it doesn't support setting a
     * particular Metadata field.
     *
     * The framework will attempt to set the following StandardMetadataType
     * values: DATASPACE, SMPTE2086, CTA861_3, and BLEND_MODE.
     * We require everyone to support setting those fields. Framework will also attempt to set
     * SMPTE2094_40 and SMPTE2094_10 if available, and it is required to support setting those
     * if it is possible to get them. If a device's Composer implementation supports a field,
     * it should be supported here. Over time these metadata fields will be moved out of
     * Composer/BufferQueue/etc. and into the buffer's Metadata fields.
     *
     * @param buffer Buffer receiving desired metadata
     * @param metadataType MetadataType for the metadata value being set
     * @param metadata Pointer to a buffer of bytes representing the value associated with
     * @param metadataSize The size of the metadata buffer
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the raw handle is invalid.
     *     - `BAD_VALUE` when the field is constant and can never be set (such as
     *       BUFFER_ID, NAME, WIDTH, HEIGHT, LAYER_COUNT, PIXEL_FORMAT_REQUESTED and
     *       USAGE)
     *     - `NO_RESOURCES` if the set cannot be fulfilled due to unavailability of
     *        resources.
     *     - `UNSUPPORTED` when metadataType is unknown/unsupported or setting
     *       it is unsupported. Unsupported should also be returned if the metadata
     *       is malformed.
     */
    AIMapper_Error (*_Nonnull setMetadata)(buffer_handle_t _Nonnull buffer,
                                           AIMapper_MetadataType metadataType,
                                           const void* _Nonnull metadata, size_t metadataSize);

    /**
     * Sets the global value for a given MetadataType.
     *
     * This is equivalent to `setMetadata` when passed an AIMapper_MetadataType with name
     * set to "android.hardware.graphics.common.StandardMetadataType"
     *
     * Metadata fields are not required to be settable. This function can
     * return Error::UNSUPPORTED whenever it doesn't support setting a
     * particular Metadata field.
     *
     * The framework will attempt to set the following StandardMetadataType
     * values: DATASPACE, SMPTE2086, CTA861_3, and BLEND_MODE.
     * We require everyone to support setting those fields. Framework will also attempt to set
     * SMPTE2094_40 and SMPTE2094_10 if available, and it is required to support setting those
     * if it is possible to get them. If a device's Composer implementation supports a field,
     * it should be supported here. Over time these metadata fields will be moved out of
     * Composer/BufferQueue/etc. and into the buffer's Metadata fields.
     *
     * @param buffer Buffer receiving desired metadata
     * @param standardMetadataType StandardMetadataType for the metadata value being set
     * @param metadata Pointer to a buffer of bytes representing the value associated with
     * @param metadataSize The size of the metadata buffer
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the raw handle is invalid.
     *     - `BAD_VALUE` when the field is constant and can never be set (such as
     *       BUFFER_ID, NAME, WIDTH, HEIGHT, LAYER_COUNT, PIXEL_FORMAT_REQUESTED and
     *       USAGE)
     *     - `NO_RESOURCES` if the set cannot be fulfilled due to unavailability of
     *        resources.
     *     - `UNSUPPORTED` when metadataType is unknown/unsupported or setting
     *       it is unsupported. Unsupported should also be returned if the metadata
     *       is malformed.
     */
    AIMapper_Error (*_Nonnull setStandardMetadata)(buffer_handle_t _Nonnull buffer,
                                                   int64_t standardMetadataType,
                                                   const void* _Nonnull metadata,
                                                   size_t metadataSize);

    /**
     * Lists all the MetadataTypes supported by IMapper as well as a description
     * of each supported MetadataType. For StandardMetadataTypes, the description
     * string can be left empty.
     *
     * This list is expected to be static & thus the returned array must be valid for the
     * lifetime of the process.
     *
     * @param outDescriptionList The list of descriptions
     * @param outNumberOfDescriptions How many descriptions are in `outDescriptionList`
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `UNSUPPORTED` if there's any error
     */
    AIMapper_Error (*_Nonnull listSupportedMetadataTypes)(
            const AIMapper_MetadataTypeDescription* _Nullable* _Nonnull outDescriptionList,
            size_t* _Nonnull outNumberOfDescriptions);

    /**
     * Dumps a buffer's metadata.
     *
     * @param buffer The buffer to dump the metadata for
     * @param dumpBufferCallback Callback that will be invoked for each of the metadata fields
     * @param context A caller-provided context to be passed to the dumpBufferCallback
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the raw handle is invalid.
     *     - `NO_RESOURCES` if the get cannot be fulfilled due to unavailability of
     *       resources.
     */
    AIMapper_Error (*_Nonnull dumpBuffer)(buffer_handle_t _Nonnull buffer,
                                          AIMapper_DumpBufferCallback _Nonnull dumpBufferCallback,
                                          void* _Null_unspecified context);

    /**
     * Dump the metadata for all imported buffers in the current process
     *
     * The HAL implementation should invoke beginDumpCallback before dumping a buffer's metadata,
     * followed by N calls to dumpBufferCallback for that buffer's metadata fields. The call
     * sequence should follow this pseudocode:
     *
     * for (auto buffer : gListOfImportedBuffers) {
     *    beginDumpCallback(context);
     *    for (auto metadata : buffer->allMetadata()) {
     *        dumpBufferCallback(context, metadata...);
     *    }
     * }
     *
     * @param beginDumpCallback Signals that a buffer is about to be dumped
     * @param dumpBufferCallback Callback that will be invoked for each of the metadata fields
     * @param context A caller-provided context to be passed to beginDumpCallback and
     *                dumpBufferCallback
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the raw handle is invalid.
     *     - `NO_RESOURCES` if the get cannot be fulfilled due to unavailability of
     *       resources.
     */
    AIMapper_Error (*_Nonnull dumpAllBuffers)(
            AIMapper_BeginDumpBufferCallback _Nonnull beginDumpCallback,
            AIMapper_DumpBufferCallback _Nonnull dumpBufferCallback,
            void* _Null_unspecified context);

    /**
     * Returns the region of shared memory associated with the buffer that is
     * reserved for client use.
     *
     * The shared memory may be allocated from any shared memory allocator.
     * The shared memory must be CPU-accessible and virtually contiguous. The
     * starting address must be word-aligned.
     *
     * This function may only be called after importBuffer() has been called by the
     * client. The reserved region must remain accessible until freeBuffer() has
     * been called. After freeBuffer() has been called, the client must not access
     * the reserved region.
     *
     * This reserved memory may be used in future versions of Android to
     * help clients implement backwards compatible features without requiring
     * IAllocator/IMapper updates.
     *
     * @param buffer Imported buffer handle.
     * @param outReservedRegion CPU-accessible pointer to the reserved region
     * @param outReservedSize the size of the reservedRegion that was requested
     *    in the BufferDescriptorInfo.
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the buffer is invalid.
     */
    AIMapper_Error (*_Nonnull getReservedRegion)(buffer_handle_t _Nonnull buffer,
                                                 void* _Nullable* _Nonnull outReservedRegion,
                                                 uint64_t* _Nonnull outReservedSize);

} AIMapperV5;

/**
 * Return value for AIMapper_loadIMapper
 *
 * Note: This struct's size is not fixed and callers must never store it by-value as a result.
 *       Only fields up to those covered by `version` are allowed to be accessed.
 */
typedef struct AIMapper {
    alignas(alignof(max_align_t)) AIMapper_Version version;
    AIMapperV5 v5;
} AIMapper;

/**
 * Loads the vendor-provided implementation of AIMapper
 * @return Error status of the call.
 *          - `NONE` upon success
 *          - `UNSUPPORTED` if no implementation is available
 */
AIMapper_Error AIMapper_loadIMapper(AIMapper* _Nullable* _Nonnull outImplementation);

__END_DECLS