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

import android.os.ParcelFileDescriptor;

/** Contains the eventfds used to notify the producer or a consumer endpoint on a data flow. */
@VintfStability
parcelable DataFlowNotificationFds {
    /**
     * The eventfd used to send waking notifications to this endpoint. EFD_SEMAPHORE must not be
     * set. The notifier will write the notification count (usually 1) to the eventfd, incrementing
     * the internal counter by the same amount. If the listener is on the host (i.e. the HAL is the
     * notifier), the listener will read the current eventfd value, then write the same value to the
     * {@link #halAck} to indicate the number of waking notifications that have been received.
     * Otherwise, the listener (the HAL) will discard the value read from it.
     */
    ParcelFileDescriptor waking;

    /**
     * The eventfd the used to send non-waking notifications to this endpoint. The listener may
     * discard the value read.
     */
    ParcelFileDescriptor nonWaking;

    /**
     * The eventfd used to acknowledge notifications on {@link #waking}. This will be non-null if
     * and only if this is a host endpoint.
     */
    @nullable ParcelFileDescriptor halAck;
}
