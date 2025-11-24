/*
 * Copyright (C) 2024 The Android Open Source Project
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

import android.hardware.wifi.supplicant.DeviceIdentityKey;
import android.hardware.wifi.supplicant.UsdServiceProtoType;

/**
 * USD data used in both publish and subscribe configurations.
 */
@VintfStability
parcelable UsdBaseConfig {
    /**
     * Service name of the USD session. A UTF-8 encoded string from 1 to 255 bytes in length.
     * The only acceptable single-byte UTF-8 symbols for a Service Name are alphanumeric
     * values (A-Z, a-z, 0-9), hyphen ('-'), period ('.'), and underscore ('_'). All
     * valid multi-byte UTF-8 characters are acceptable in a Service Name.
     */
    @utf8InCpp String serviceName;

    /**
     * Service protocol type for the USD session (ex. Generic, CSA Matter).
     */
    UsdServiceProtoType serviceProtoType;

    /**
     * Details about the service being offered or being looked for. This information is transmitted
     * within Service Discovery frames, and is used to help devices find each other and establish
     * connections. The format and content of the service specific information are flexible and
     * can be determined by the application.
     */
    byte[] serviceSpecificInfo;

    /**
     * Ordered sequence of <length, value> pairs (|length| uses 1 byte and contains the number of
     * bytes in the |value| field) which specify further match criteria (beyond the service name).
     *
     * The match behavior is specified in details in the NAN spec.
     * Publisher: used if provided.
     * Subscriber: used (if provided) only in ACTIVE sessions.
     *
     * Max length: |UsdCapabilities.maxMatchFilterLength|.
     * NAN Spec: matching_filter_tx and Service Descriptor Attribute (SDA) / Matching Filter
     */
    @nullable byte[] txMatchFilter;

    /**
     * Ordered sequence of <length, value> pairs (|length| uses 1 byte and contains the number of
     * bytes in the |value| field) which specify further match criteria (beyond the service name).
     *
     * The match behavior is specified in details in the NAN spec.
     * Publisher: used in SOLICITED or SOLICITED_UNSOLICITED sessions.
     * Subscriber: used in ACTIVE or PASSIVE sessions.
     *
     * Max length: |UsdCapabilities.maxMatchFilterLength|.
     * NAN Spec: matching_filter_rx
     */
    @nullable byte[] rxMatchFilter;

    /**
     * Time interval (in seconds) that a USD session will be alive.
     * The session will be terminated when the time to live (TTL) is reached, triggering either
     * |ISupplicantStaIfaceCallback.onUsdPublishTerminated| for Publish, or
     * |ISupplicantStaIfaceCallback.onUsdSubscribeTerminated| for Subscribe.
     */
    int ttlSec;

    /**
     * Frequency where the device should begin to dwell. Default value is channel 6 (2.437 GHz),
     * but other values may be selected per regulation in the geographical location.
     */
    int defaultFreqMhz;

    /**
     * Channels which can be switched to. May contain any of the 20 MHz channels in the
     * 2.4 Ghz and/or 5 Ghz bands, per regulation in the geographical location.
     */
    int[] freqsMhz;

    /**
     * Whether the proximity ranging is enabled.
     */
    boolean isRangingEnabled;

    /**
     * This Ranging device's identity key (devIK) required for authenticated PASN mode in
     * proximity ranging.
     *
     * As per the specification, a device seeking proximity ranging with Authenticated mode PASN
     * security setup (section 4.2) shall use the configured Device Identity-Key (DevIK) as a
     * long-term device identity to create a DIRA attribute (PR Device Identity Resolution attribute
     * section 3.2.8) and include it in the USD service discovery frames the device sends. When a
     * Device receives a DIRA from another P2P Device, it derives a set of Tag values based on the
     * cached DevIKs of all known peers for proximity ranging. If a derived Tag value matches the
     * Tag value in the received DIRA, the Device identifies the transmitter of the DIRA as a known
     * peer.
     *
     */
    @nullable DeviceIdentityKey selfDevIk;

    /**
     * List of peer device's device identity key
     * When wpa_supplicant receives the DIRA attribute, it goes through this list of DevIKs and
     * verify if it's a known peer. If it's a known peer, the devIk will be added in the
     * discovery result.
     */
    @nullable DeviceIdentityKey[] peerDevIks;
}
