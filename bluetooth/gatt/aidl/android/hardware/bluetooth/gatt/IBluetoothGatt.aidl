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

import android.hardware.bluetooth.gatt.GattCapabilities;
import android.hardware.bluetooth.gatt.GattSession;
import android.hardware.bluetooth.gatt.IBluetoothGattCallback;

/**
 * The interface for host stack to register callback, to get capabilities, to offload
 * GATT service, and to unoffload GATT service.
 */
@VintfStability
interface IBluetoothGatt {
    /**
     * Error codes that are used as service specific errors with the AIDL return
     * value EX_SERVICE_SPECIFIC.
     */
    const int EX_BLUETOOTH_GATT_UNSPECIFIED = -1;

    /**
     * API to initialize the GATT HAL and to register a callback for receiving asynchronous
     * events.
     *
     * This method is the entry point for interacting with the GATT hardware abstraction layer.
     * Subsequent calls to this method with a different callback object must replace the previously
     * registered one.
     *
     * @param callback An instance of the |IBluetoothGattCallback| AIDL interface object
     *
     * @throws EX_ILLEGAL_ARGUMENT If any of the parameters are invalid.
     */
    void init(in IBluetoothGattCallback callback);

    /**
     * API to retrieve the supported GATT offload capabilities.
     *
     * This method allows to query the underlying low power processor and HAL implementation
     * to determine which GATT offload features are supported on this device.
     *
     * @return A {@link GattCapabilities} object detailing the specific GATT offload features
     * supported by the low power processor.
     */
    GattCapabilities getGattCapabilities();

    /**
     * API to offload the GATT service to the endpoint for either GATT client or server.
     *
     * This message allows the GATT app to delegate the handling of a subset of the characteristics
     of a GATT service to the endpoint.
     *
     * When used for a GATT client role, this operation must be performed after a service
     * discovery from remote GATT server. This is crucial because all subsequent GATT operations
     depend on the attribute handles of the characteristics. These handles are dynamically assigned
     by the GATT server and are obtained exclusively during the service discovery process.
     *
     * @param session Parameters for the GATT offload session to be registered.
     *
     * @throws EX_ILLEGAL_ARGUMENT If any of the parameters in {@code session} are invalid.
     * @throws EX_UNSUPPORTED_OPERATION If the operation is not supported.
     * @throws EX_SERVICE_SPECIFIC on other errors
               - EX_BLUETOOTH_GATT_UNSPECIFIED if the request failed for other reasons.
     */
    void registerService(in GattSession session);

    /**
     * API to unregister a previously offloaded GATT offload session or signal its closure.
     *
     * This API can be invoked under several circumstances, including when the host
     * application explicitly requests to unregister GATT offload session, when the
     * underlying channel disconnects, or when the GATT client or server is unregistered by
     * the Host application.
     *
     * @param sessionId The unique identifier for the GATT session that was previously
     *        assigned when the service was offloaded
     *
     * @throws EX_ILLEGAL_ARGUMENT If any of the parameters are invalid.
     * @throws EX_UNSUPPORTED_OPERATION If the operation is not supported.
     * @throws EX_SERVICE_SPECIFIC on other errors
               - EX_BLUETOOTH_GATT_UNSPECIFIED if the request failed for other reasons.
     */
    void unregisterService(in int sessionId);

    /**
     * Requests the offload stack to clear the database of offloaded characteristics
     * for the selected ACL Connection.
     *
     * This method is invoked when the host Bluetooth stack detects that the remote GATT
     * server's database has changed and is no longer synchronized with the local copy.
     *
     * The offload stack must ensure that no pending ATT procedure exists for the selected
     * ACL connect before reporting completion through
     * IBluetoothGattCallback.clearServicesComplete().
     *
     * This API will be called when the host stack becomes aware of a change in the remote
     * database through DATABASE_OUT_OF_SYNC errors or Service Change notifications.
     *
     * @param aclConnectionHandle  Handle of the selected ACL connection
     *
     * @throws EX_ILLEGAL_ARGUMENT If any of the parameters are invalid.
     * @throws EX_UNSUPPORTED_OPERATION If the operation is not supported.
     * @throws EX_SERVICE_SPECIFIC on other errors
               - EX_BLUETOOTH_GATT_UNSPECIFIED if the request failed for other reasons.
     */
    void clearServices(in int aclConnectionHandle);
}
