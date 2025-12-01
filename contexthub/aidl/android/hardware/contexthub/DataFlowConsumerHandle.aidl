/*
 * Copyright (C) 2025 The Android Open Source Project
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

import android.hardware.common.NativeHandle;
import android.hardware.contexthub.DataFlowId;
import android.hardware.contexthub.DataFlowInfo;
import android.hardware.contexthub.DataFlowNotificationFds;
import android.hardware.contexthub.SharedDataRegion;
import android.os.ParcelFileDescriptor;

/**
 * Contains the necessary information to initialize a data flow consumer.
 *
 * NOTE: This parcelable is both sent to the HAL and received by HAL clients. Since the HAL can look
 * up much of the relevant information, some of the fields will be left empty to avoid unnecessary
 * fd duplication and input validation.
 */
@VintfStability
parcelable DataFlowConsumerHandle {
    /** The id associated with this data flow. */
    DataFlowId id;

    /**
     * The common details of the data flow including the primary shared data region, producer
     * metadata offset, and producer eventfd. Must be set by the HAL when sending to clients. Must
     * be null when sending to the HAL i.e. the data flow must have already been registered with the
     * HAL.
     */
    @nullable DataFlowInfo info;

    /**
     * A shared data region containing the consumer descriptor. On devices supporting memory
     * protection in the offload region, consumers must have write access only to their own
     * descriptors and read access to producer state and the flow's data. On these devices,
     * info.region is read-only to the consumer, while this region is writable to both the producer
     * and this consumer. The producer has write access in order to set flags in the consumer
     * descriptor. See {@link #SharedDataRegion} for a description of the memory format and how the
     * backwards compatibility is maintained.
     *
     * Must be null when a client calls IEndpointCommunication::registerDataFlowOffloadConsumer().
     * The HAL will use
     * IEndpointCommunication::IRegisterOffloadConsumerNestedCallback::addConsumerInRegion() to
     * provide a dedicated region for allocating the consumer descriptor (if supported by the
     * device) within the same call.
     */
    @nullable SharedDataRegion consumerRegion;

    /**
     * The offset in bytes of the consumer descriptor from the start of consumerRegion if provided,
     * otherwise info.region.
     *
     * The HAL must ignore this field when a client calls
     * IEndpointCommunication::registerDataFlowOffloadConsumer(). The client must instead provide
     * this as the return value of
     * IEndpointCommunication::IRegisterOffloadConsumerNestedCallback::addConsumerInRegion().
     */
    long metadataOffsetBytes;

    /** The eventfds to propagate notifications from the data flow producer to this consumer. */
    DataFlowNotificationFds notificationFds;
}
