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

@VintfStability
parcelable ErrorReport {
    /** Reported error type in errorReport. */
    @VintfStability
    @Backing(type="int")
    enum Error {
        /**
         * Default value. This value means Error wasn't explicitly initialized and must be discarded
         * by the host stack.
         */
        UNKNOWN,

        /**
         * Indicates that an ATT Error Response PDU was received with the error code 0x12
         * (Database Out Of Sync). The host stack is responsible for reading the database hash and
         * initiating the service discovery procedure to update the database. Host applications are
         * responsible for registering the services again after the GATT service discovery procedure
         * is complete.
         */
        DATABASE_OUT_OF_SYNC,

        /**
         * Indicates that the remote device failed to respond within the expected time.
         * The host stack is required to disconnect the underlying ACL link or EATT
         * channel for this connection.
         */
        RESPONSE_TIMEOUT,

        /**
         * Indicates a protocol violation occurred. The host stack is required to
         * disconnect the underlying ACL link or logical channel for this session.
         */
        PROTOCOL_VIOLATION,
    }

    /**
     * Handle of the ACL connection where the error occurred.
     */
    int aclConnectionHandle;

    /**
     * Local Channel ID used for the connection.
     */
    int localCid;

    /**
     * The type of error that occurred.
     */
    Error error;

    /**
     * A human-readable string providing more details about the error.
     */
    String reason;
}
