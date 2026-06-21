/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.contexthub;

import android.hardware.contexthub.DataFlowId;
import android.hardware.contexthub.DataFlowInfo;
import android.hardware.contexthub.DataFlowSinkRegistrationParams;
import android.hardware.contexthub.EndpointId;
import android.hardware.contexthub.EndpointInfo;
import android.hardware.contexthub.IEndpointCallback;
import android.hardware.contexthub.Message;
import android.hardware.contexthub.MessageDeliveryStatus;
import android.hardware.contexthub.Reason;
import android.hardware.contexthub.Service;
import android.hardware.contexthub.SharedDataCapabilities;
import android.hardware.contexthub.SharedDataRegion;
import android.hardware.contexthub.SharedDataRegionRequirements;

@VintfStability
interface IEndpointCommunication {
    /** Invalid session id. */
    const int SESSION_ID_INVALID = -1;

    /**
     * Publishes an endpoint from the calling side (e.g. Android). Endpoints must be registered
     * prior to starting a session.
     */
    void registerEndpoint(in EndpointInfo endpoint);

    /**
     * Teardown an endpoint from the calling side (e.g. Android). This endpoint must have already
     * been published via registerEndpoint().
     */
    void unregisterEndpoint(in EndpointInfo endpoint);

    /**
     * Request a range of session IDs for the caller to use when initiating sessions. This may be
     * called more than once, but typical usage is to request a large enough range to accommodate
     * the maximum expected number of concurrent sessions, but not overly large as to limit other
     * clients.
     *
     * @param size The number of sessionId reserved for host-initiated sessions. This number should
     *         be less than or equal to 1024.
     *
     * @return An array with two elements representing the smallest and largest possible session id
     *         available for host.
     *
     * @throws EX_ILLEGAL_ARGUMENT if the size is invalid.
     * @throws EX_SERVICE_SPECIFIC if the id range requested cannot be allocated.
     */
    int[2] requestSessionIdRange(int size);

    /**
     * Request to open a session for communication between an endpoint previously registered by the
     * caller and a target endpoint found in getEndpoints(), optionally scoped to a service
     * published by the target endpoint.
     *
     * Upon returning from this function, the session is in pending state, and the final result will
     * be given by an asynchronous call to onEndpointSessionOpenComplete() on success, or
     * onCloseEndpointSession() on failure. If a call to onEndpointSessionOpenComplete() is not
     * received within 10 seconds, the session will be considered failed and
     * onCloseEndpointSession() should be called with reason Reason.TIMEOUT.
     *
     * @param sessionId Caller-allocated session identifier, which must be unique across all active
     *         sessions, and must fall in a range allocated via requestSessionIdRange().
     * @param destination The EndpointId representing the destination side of the session.
     * @param initiator The EndpointId representing the initiating side of the session, which
     *         must've already been published through registerEndpoint().
     * @param serviceDescriptor Descriptor for the service specification for scoping this session
     *         (nullable). Null indicates a fully custom marshalling scheme. The value should match
     *         a published descriptor for both destination and initiator.
     *
     * @throws EX_ILLEGAL_ARGUMENT if any of the arguments are invalid, or the combination of the
     *         arguments is invalid.
     * @throws EX_SERVICE_SPECIFIC on other errors
     *         - EX_CONTEXT_HUB_UNSPECIFIED if the request failed for other reasons.
     */
    void openEndpointSession(int sessionId, in EndpointId destination, in EndpointId initiator,
            in @nullable String serviceDescriptor);

    /**
     * Send a message from one endpoint to another on the (currently open) session.
     *
     * @param sessionId The integer representing the communication session, previously set in
     *         openEndpointSession() or onEndpointSessionOpenRequest().
     * @param msg The Message object representing a message to endpoint from the endpoint on host.
     *
     * @throws EX_ILLEGAL_ARGUMENT if any of the arguments are invalid, or the combination of the
     *         arguments is invalid.
     * @throws EX_SERVICE_SPECIFIC on other errors
     *         - EX_CONTEXT_HUB_UNSPECIFIED if the request failed for other reasons.
     */
    void sendMessageToEndpoint(int sessionId, in Message msg);

    /**
     * Sends a message delivery status to the endpoint in response to receiving a Message with flag
     * FLAG_REQUIRES_DELIVERY_STATUS. Each message with the flag should have a MessageDeliveryStatus
     * response. This method sends the message delivery status back to the remote endpoint for a
     * session.
     *
     * @param sessionId The integer representing the communication session, previously set in
     *         openEndpointSession() or onEndpointSessionOpenRequest().
     * @param msgStatus The MessageDeliveryStatus object representing the delivery status for a
     *         specific message (identified by the sequenceNumber) within the session.
     *
     * @throws EX_UNSUPPORTED_OPERATION if ContextHubInfo.supportsReliableMessages is false for
     *          the hub involved in this session.
     */
    void sendMessageDeliveryStatusToEndpoint(int sessionId, in MessageDeliveryStatus msgStatus);

    /**
     * Closes a session previously opened by openEndpointSession() or requested via
     * onEndpointSessionOpenRequest(). Processing of session closure must be ordered/synchronized
     * with message delivery, such that if this session was open, any messages previously passed to
     * sendMessageToEndpoint() that are still in-flight must still be delivered before the session
     * is closed. Any in-flight messages to the endpoint that requested to close the session will
     * not be delivered.
     *
     * @param sessionId The integer representing the communication session, previously set in
     *         openEndpointSession() or onEndpointSessionOpenRequest().
     * @param reason The reason for this close endpoint session request.
     *
     * @throws EX_ILLEGAL_ARGUMENT if any of the arguments are invalid, or the combination of the
     *         arguments is invalid.
     * @throws EX_SERVICE_SPECIFIC on other errors
     *         - EX_CONTEXT_HUB_UNSPECIFIED if the request failed for other reasons.
     */
    void closeEndpointSession(int sessionId, in Reason reason);

    /**
     * Notifies the HAL that the session requested by onEndpointSessionOpenRequest is ready to use.
     *
     * @param sessionId The integer representing the communication session, previously set in
     *         onEndpointSessionOpenRequest(). This id is assigned by the HAL.
     *
     * @throws EX_ILLEGAL_ARGUMENT if any of the arguments are invalid, or the combination of the
     *         arguments is invalid.
     * @throws EX_SERVICE_SPECIFIC on other errors
     *         - EX_CONTEXT_HUB_UNSPECIFIED if the request failed for other reasons.
     */
    void endpointSessionOpenComplete(int sessionId);

    /**
     * Unregisters this hub. Subsequent calls on this interface will fail.
     *
     * @throws EX_ILLEGAL_STATE if this interface was already unregistered.
     */
    void unregister();

    // The following methods are used to create efficient data flows from host to offload endpoints
    // within shared data regions. The following is the sequence of operations to create one or more
    // data flows in a region:
    // 1. Use allocateSharedDataRegion() to create a new region.
    // 2. Use registerDataFlowHostSource() to register a data flow within the region. This can be
    //    called multiple times to register multiple data flows within the same region.
    // 3. Use registerDataFlowOffloadSink() to share a new sink context for that data flow with
    //    an offload endpoint. This can be called multiple times to share the same data flow with
    //    multiple offload endpoints.
    // 4. Use unregisterDataFlowHostSource() to unregister a data flow.
    // 5. Use freeSharedDataRegion() to release the region. The HAL will reject this request if any
    //    data flows registered by host endpoints have not been unregistered.
    //
    // For data flows from offload to host endpoints, the host endpoint receives sink access to
    // a data flow from the IEndpointCallback::onDataFlowHostSinkRegistered() callback, which
    // includes the context needed to access the shared data region. When done with the data flow,
    // the sink calls unregisterDataFlowHostSink() to release the HAL resources associated with the
    // sink's access to the data flow.

    /** Error codes for shared data region operations. */
    @VintfStability
    enum SharedDataErrors {
        ERR_INSUFFICIENT_MEMORY = 1,
        ERR_INVALID_CONFIGURATION = 2
    }

    /**
     * Requests the allocation of a new shared data region writable by a host endpoint. This region
     * can support data flows from a host source to offload sinks.
     *
     * @param requirements The requirements for the allocation, including size.
     *
     * @return The newly allocated region.
     *
     * @throws EX_ILLEGAL_ARGUMENT if any of the requirements are invalid.
     * @throws EX_UNSUPPORTED_OPERATION if shared data regions are not supported.
     * @throws EX_SERVICE_SPECIFIC on other errors
     *         - ERR_INSUFFICIENT_MEMORY if the request failed due to insufficient memory.
     *         - ERR_INVALID_CONFIGURATION if the request failed due to invalid configuration, e.g.
     *           the set of target hubs does not have a common shared memory region.
     */
    SharedDataRegion allocateSharedDataRegion(in SharedDataRegionRequirements requirements);

    /**
     * Frees a previously allocated shared data region.
     *
     * @param id A {@link SharedDataRegion#id} previously returned by {@code
               allocateSharedDataRegion()}. Must not have been freed already.
     *
     * @throws EX_ILLEGAL_ARGUMENT if the id wasn't previously successfully assigned to a region by
     *         {@code allocateSharedDataRegion()}.
     * @throws EX_ILLEGAL_STATE if the region is in use.
     * @throws EX_UNSUPPORTED_OPERATION if shared data regions are not supported.
     */
    void freeSharedDataRegion(int id);

    /**
     * Registers a new data flow in the given shared data region. The HAL stores the DataFlowInfo
     * to track shared data region usage, route notifications to the source, and share the data
     * flow with sinks.
     *
     * @param endpoint The source endpoint for this data flow.
     * @param info The information about the data flow to register.
     *
     * @return An id scoped to this message hub representing the new data flow.
     *
     * @throws EX_ILLEGAL_ARGUMENT if the region doesn't exist or is not active, or if the data flow
     *         metadata offset is invalid.
     * @throws EX_UNSUPPORTED_OPERATION if shared data regions are not supported.
     */
    int registerDataFlowHostSource(in EndpointId endpoint, in DataFlowInfo info);

    /**
     * Unregisters the data flow with given id. The HAL releases its references to the associated
     * shared data region(s) and eventfds. It sends a final notification to relevant offload message
     * hubs indicating that the memory associated with the data flow will be repurposed.
     *
     * To ensure that sinks safely stop accessing the data flow before tear down, the source
     * endpoint must alert sinks that the data flow is being stopped and wait for acknowledgement
     * before calling this API. This can be done either through the data flow implementation (using
     * shared memory and the alert eventfds) or an out-of-band mechanism like a session message.
     *
     * @param id An id previously returned by {@code registerDataFlowHostSource()}. Must not have
               been unregistered already.
     *
     * @throws EX_ILLEGAL_ARGUMENT if the id is unknown.
     * @throws EX_UNSUPPORTED_OPERATION if shared data regions are not supported.
     */
    void unregisterDataFlowHostSource(int id);

    /**
     * Sends the context for a new sink on a data flow on this hub to an offload endpoint.
     *
     * The HAL will call {@link IRegisterOffloadSinkCallback#addSinkInRegion(SharedDataRegion)}
     * from within the thread servicing this API i.e. it will be called before this API returns and
     * Binder ensures that the calling thread will handle the nested transaction. The
     * implementation of {@code addSinkInRegion()} will actually allocate the sink metadata and
     * return its offset to the HAL.
     *
     * @param params The parameters for registering a new sink on a data flow.
     * @param callback The callback to provide additional information to the HAL within this call.
     *
     * @throws EX_ILLEGAL_ARGUMENT if the data flow doesn't exist or is not active, or if the
     *         sink context is invalid.
     * @throws EX_UNSUPPORTED_OPERATION if shared data regions are not supported.
     * @throws EX_SERVICE_SPECIFIC on other errors
     *         - ERR_INSUFFICIENT_MEMORY if the dedicated sink metadata region cannot be allocated.
     */
    void registerDataFlowOffloadSink(
            in DataFlowSinkRegistrationParams params, in IRegisterOffloadSinkCallback callback);

    /**
     * Releases HAL resources associated with the calling endpoint reading from a data flow
     * received via {@link IEndpointCallback#onDataFlowHostSinkRegistered()}. This will be called
     * after the endpoint stops reading from the data flow. This API will not directly result in an
     * alert to the source endpoint. If desired, the sink can alert the source via the data flow
     * implementation or an out-of-band mechanism.
     *
     * The framework will also call this API to notify the HAL when a sink context cannot be passed
     * to a host endpoint due to insufficient permissions.
     *
     * @param sinkId The endpoint reading from the data flow.
     * @param dataFlowId The id of the data flow to release.
     *
     * @throws EX_ILLEGAL_ARGUMENT if the data flow doesn't exist.
     * @throws EX_UNSUPPORTED_OPERATION if shared data regions are not supported.
     */
    void unregisterDataFlowHostSink(in EndpointId sinkId, in DataFlowId dataFlowId);

    /**
     * An interface for nesting callbacks from the HAL to the client within IEndpointCommunication
     * calls. This is used to provide additional information to the HAL within a single RPC call.
     */
    @VintfStability
    interface IRegisterOffloadSinkCallback {
        /**
         * Provides an optional region for allocating the sink metadata. This must only be called
         * within the thread servicing registerDataFlowOffloadSink().
         *
         * NOTE: The returned offset is a {@code long} to allow for future support of regions larger
         * than 4GB. That will necessitate a major version bump and new ABI structures defined in
         * {@link SharedDataRegion}.
         *
         * @param region The shared data region to allocate the sink metadata from. If null,
         *         the metadata will be allocated from the primary region returned by {@link
         *         #allocateSharedDataRegion()}. Otherwise, the descriptor will be allocated in the
         *         given region.
         * @return The offset in bytes of the sink metadata from the start of the provided region if
         *         not null or in the primary region.
         */
        long addSinkInRegion(in @nullable SharedDataRegion region);
    }
}
