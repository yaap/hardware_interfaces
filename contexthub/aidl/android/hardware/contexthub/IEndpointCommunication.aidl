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

import android.hardware.contexthub.EndpointId;
import android.hardware.contexthub.EndpointInfo;
import android.hardware.contexthub.IEndpointCallback;
import android.hardware.contexthub.Message;
import android.hardware.contexthub.MessageDeliveryStatus;
import android.hardware.contexthub.Reason;
import android.hardware.contexthub.Service;

@VintfStability
interface IEndpointCommunication {
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
}
