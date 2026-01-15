/*
 * Copyright (C) 2026 The Android Open Source Project
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

import android.hardware.contexthub.DataFlowSinkContext;
import android.hardware.contexthub.EndpointId;
import android.hardware.contexthub.Message;

/**
 * Parameters for registering a data flow sink. Used both to share a data flow with an offload
 * endpoint and to receive access to a data flow whose source is an offload endpoint.
 */
@VintfStability
parcelable DataFlowSinkRegistrationParams {
    /** The handle used to give the new consumer access to the data flow. */
    DataFlowSinkContext context;

    /** The endpoint which is sending the handle. */
    EndpointId sourceId;

    /** The endpoint which will read from the data flow. */
    EndpointId sinkId;

    /** An optional message sent by the offload endpoint. */
    @nullable Message msg;

    /**
     * An optional open session id between the data flow producer and the destination endpoint
     * to associate this call with. If msg is provided, this session can be used to send a
     * MessageDeliveryStatus in response. Ignored if set to {@link
     * IEndpointCommunication#SESSION_ID_INVALID}.
     */
    int sessionId;
}