/**
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

package android.hardware.wifi.supplicant;

import android.hardware.wifi.supplicant.ISupplicantWifiRttControllerEventCallback;
import android.hardware.wifi.supplicant.IfaceInfo;
import android.hardware.wifi.supplicant.MacAddress;
import android.hardware.wifi.supplicant.RttCapabilities;
import android.hardware.wifi.supplicant.RttConfig;

/**
 * Interface used to perform RTT (Round trip time) operations.
 */
@VintfStability
interface ISupplicantWifiRttController {
    /**
     * Retrieves the name of the network interface attached via
     * |ISupplicantStaIface.createRttController|.
     *
     * @return Name of the network interface, e.g., wlan0
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    String getName();

    /**
     * RTT capabilities of the device.
     *
     * @return Instance of |RttCapabilities|.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.ERROR_WIFI_RTT_CONTROLLER_INVALID|,
     *         |SupplicantStatusCode.ERROR_UNKNOWN|
     */
    RttCapabilities getCapabilities();

    /**
     * Set the device name for Proximity Ranging.
     * User-friendly name of the Proximity Ranging device
     * (up to 32 bytes encoded in UTF-8).
     *
     * @param name Name to be set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setProximityRangingDeviceName(in String name);

    /**
     * Changes the MAC address used for proximity ranging.
     * Note: The MAC address will be used for USD discovery with
     * ranging enabled, Proximity Ranging security/range/channel
     * negotiation and range measurements.
     *
     * @param macAddress MAC address to change to.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |SupplicantStatusCode.ERROR_UNKNOWN|
     */
    void setProximityRangingMacAddress(in byte[6] macAddress);

    /**
     * Get the MAC address which will be used in security/range
     * role/channel negotiation & range measurement.
     *
     * @return The MAC address of the interface used for USD discovery with
     * ranging enabled, Proximity Ranging security/range/channel
     * negotiation and range measurements.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.ERROR_UNKNOWN|
     */
    byte[6] getProximityRangingMacAddress();

    /**
     * API to request RTT measurement. The response for the range request is
     * received via |ISupplicantWifiRttControllerEventCallback.onResults|,
     * |ISupplicantWifiRttControllerEventCallback.onContinuousRangingStatusChanged| and
     * |ISupplicantWifiRttControllerEventCallback.onContinuousRangingTerminated| callbacks.
     * The callback should be registered using |registerEventCallback| to get this response.
     *
     * @param cmdId Command Id to use for this invocation. The caller is responsible for
     *              ensuring uniqueness of this ID for this RTT Controller instance.
     * @param rttConfigs Vector of |RttConfig| parameters.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.ERROR_WIFI_RTT_CONTROLLER_INVALID|,
     *         |SupplicantStatusCode.ERROR_INVALID_ARGS|,
     *         |SupplicantStatusCode.ERROR_NOT_AVAILABLE|,
     *         |SupplicantStatusCode.ERROR_UNKNOWN|
     */
    void rangeRequest(in int cmdId, in RttConfig[] rttConfigs);

    /**
     * API to cancel RTT measurements. The response for the cancel range is
     * received via |ISupplicantWifiRttControllerEventCallback.onContinuousRangingTerminated|
     * callback. The callback should be registered using |registerEventCallback| to get this
     * response.
     *
     * @param cmdId Command Id corresponding to the original request.
     * @param addrs Vector of addresses for which to cancel.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.ERROR_WIFI_RTT_CONTROLLER_INVALID|,
     *         |SupplicantStatusCode.ERROR_INVALID_ARGS|,
     *         |SupplicantStatusCode.ERROR_NOT_AVAILABLE|,
     *         |SupplicantStatusCode.ERROR_UNKNOWN|
     */
    void rangeCancel(in int cmdId, in MacAddress[] addrs);

    /**
     * Requests notifications of significant events on this RTT controller.
     * The callback should be registered to get responses for rangeRequest and rangeCancel
     * operations.
     *
     * @param callback An instance of the |ISupplicantWifiRttControllerEventCallback| AIDL
     *        interface object.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|
     */
    void registerEventCallback(in ISupplicantWifiRttControllerEventCallback callback);
}
