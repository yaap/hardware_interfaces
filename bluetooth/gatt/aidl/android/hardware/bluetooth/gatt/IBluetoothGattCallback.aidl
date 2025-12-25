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

import android.hardware.bluetooth.gatt.ErrorReport;
import android.hardware.bluetooth.gatt.Status;

/**
 * The interface from the Bluetooth offload stack to the host stack.
 */
@VintfStability
oneway interface IBluetoothGattCallback {
    /**
     * Invoked when IBluetoothGatt.registerService() has been completed.
     *
     * @param sessionId The unique identifier for the GATT session that was previously
     *        assigned when the service was offloaded
     * @param status Status indicating success or failure
     * @param reason Reason string of the operation failure for debugging purposes
     */
    void registerServiceComplete(int sessionId, in Status status, in String reason);

    /**
     * Invoked to report completion of unregisterService request, or to notify the host
     * stack that the session was closed by the offload application.
     *
     * @param sessionId The unique identifier for the GATT session that was previously
     *        assigned when the service was offloaded
     * @param reason Reason string of the operation for debugging purposes
     */
    void unregisterServiceComplete(int sessionId, in String reason);

    /**
     * Report that the offloaded services for the selected ACL connection have been cleared,
     * and that all pending ATT procedures for these services have completed.
     *
     * @param aclConnectionHandle The handle of the ACL connection on which the services are
     *        cleared
     * @param reason Reason string of the operation for debugging purposes
     */
    void clearServicesComplete(int aclConnectionHandle, in String reason);

    /**
     * Invoked when offload stack notifies host stack that a error has occurred on
     * the GATT connection. Host stack is responsible for handling the error
     * appropriately based on the type of error. See the {@link ErrorReport.Error} enum.
     *
     * @param report Details of the reported error.
     */
    void errorReport(in ErrorReport report);
}
