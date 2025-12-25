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

package android.hardware.bluetooth.gatt;

import android.hardware.bluetooth.gatt.GattCharacteristic;
import android.hardware.bluetooth.gatt.Uuid;
import android.hardware.contexthub.EndpointId;

@VintfStability
parcelable GattSession {
    /**
     * Represent GATT role. A device can act as either a {@link SERVER} or a {@link CLIENT} on
     * the GATT offload session.
     */
    @VintfStability
    @Backing(type="int")
    enum Role {
        SERVER,
        CLIENT,
    }

    /**
     * Identifier assigned to the offload session by the host stack. Used to
     * uniquely identify the offload session in other callbacks and method
     * invocations.
     */
    int sessionId;

    /**
     * Handle of the ACL connection over which the GATT service is
     * offloaded.
     */
    int aclConnectionHandle;

    /**
     * Maximum transmission unit for ATT protocol negotiated for this connection.
     */
    int attMtu;

    /**
     * GATT role (SERVER or CLIENT) for which this offload session is being established.
     */
    Role role;

    /**
     * UUID of the GATT service which {@code characteristics} are associated with
     */
    Uuid serviceUuid;

    /**
     * Characteristics to be offloaded.
     */
    GattCharacteristic[] characteristics;

    /**
     * Unique identifier for an endpoint at the offload path
     */
    EndpointId endpointId;
}
