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

/**
 * Represents the status of a GATT offload operation, indicating success or
 * specific local failure reasons.
 */
@VintfStability
@Backing(type="int")
enum Status {
    /**
     * The operation completed successfully.
     */
    SUCCESS = 0,

    /**
     * The provided endpoint ID is invalid or unknown to the system.
     * This could mean the endpoint does not exist or is not registered.
     */
    INVALID_ENDPOINT_ID,

    /**
     * The requested GATT role (e.g., client or server) is not supported
     * by the specified endpoint for this operation.
     */
    UNSUPPORTED_ROLE,

    /**
     * The system or the endpoint lacks sufficient resources (e.g., memory,
     * processing power, available connections) to fulfill the request.
     */
    INSUFFICIENT_RESOURCES,

    /**
     * A general failure occurred that does not fit into other specific error
     * categories. This typically indicates an internal error on the host
     * or endpoint side that prevented the operation from completing.
     */
    FAILURE,
}
