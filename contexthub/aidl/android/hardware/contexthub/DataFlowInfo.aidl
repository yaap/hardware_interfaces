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

import android.hardware.contexthub.DataFlowNotificationFds;
import android.hardware.contexthub.SharedDataRegion;
import android.os.ParcelFileDescriptor;

/** Contains the core data flow information common to both producer and consumer endpoints. */
@VintfStability
parcelable DataFlowInfo {
    /**
     * The primary shared data region. Contains the producer descriptor and data storage. May also
     * contain consumer descriptors, see {@link DataFlowConsumer}. When sending to the HAL, the
     * client must set the region id and set the other fields to null. When sending to clients, the
     * HAL must populate all fields.
     */
    SharedDataRegion region;

    /** The offset in bytes from the start of the region to the data flow's metadata. */
    long metadataOffsetBytes;

    /** The eventfds used to propagate notifications from consumers to the producer. */
    DataFlowNotificationFds notificationFds;

    /** A brief human-readable identifier for this data flow. Must only be used for debugging. */
    String debugName;
}