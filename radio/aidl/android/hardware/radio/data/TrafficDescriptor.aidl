/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.radio.data;

import android.hardware.radio.data.OsAppId;

/**
 * This struct represents a traffic descriptor. A valid struct must have at least one of the
 * optional values present. This is based on the definition of traffic descriptor in
 * TS 24.526 Section 5.2.
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
@RustDerive(Clone=true, Eq=true, PartialEq=true)
parcelable TrafficDescriptor {
    /**
     * DNN stands for Data Network Name and represents an APN as defined in 3GPP TS 23.003.
     */
    @nullable String dnn;
    /**
     * Indicates the OsId + OsAppId (used as category in Android).
     */
    @nullable OsAppId osAppId;

    /**
     * Connection Capability,
     *
     * as defined in 3GPP TS 124.526 table 5.2.1.
     */
    @Backing(type="byte")
    enum ConnectionCapability {
        /** Unknown connection capability. */
        UNKNOWN = 0x00u8,

        /**
         * IMS Voice + Video comprising voice, video telephony and multimedia communications over
         * IP networks. Voice, Video and SMS over IMS DNN, as well as RCS (Rich Communication
         * Services) are included in this traffic category.
         *
         * Bits: 0 0 0 0 0 0 0 1
         */
        IMS = 0x01u8,

        /**
         * MMS (Multimedia Messaging Service)
         *
         * Bits: 0 0 0 0 0 0 1 0
         */
        MMS = 0x02u8,

        /**
         * SUPL (Secure User Plane Location)
         *
         * Bits: 0 0 0 0 0 1 0 0
         */
        SUPL = 0x04u8,

        /**
         * Internet data traffic with wide availability but no critical requirements on latency
         * or data rates.
         *
         * Bits: 0 0 0 0 1 0 0 0
         */
        INTERNET = 0x08u8,

        /**
         * LCS user plane positioning
         *
         * Bits: 0 0 0 1 0 0 0 0
         */
        LCS_USER_PLANE_POSITIONING = 0x10u8,

        /**
         * Delay-tolerant, low sustained data rate IoT traffic.
         * Bits: 1 0 1 0 0 0 0 1
         */
        IOT_DELAY_TOLERANT = 0xA1u8,

        /**
         * Non-delay-tolerant, low sustained data rate IoT traffic
         * Bits: 1 0 1 0 0 0 1 0
         */
        IOT_NON_DELAY_TOLERANT = 0xA2u8,

        /**
         * Downlink streaming, characterized as downlink high data rates content and low latency.
         * Bits: 1 0 1 0 0 0 1 1
         */
        DOWNLINK_STREAMING = 0xA3u8,

        /**
         * Uplink streaming, characterized as uplink high data rates content and low latency
         * Bits: 1 0 1 0 0 1 0 0
         */
        UPLINK_STREAMING = 0xA4u8,

        /**
         * Vehicle-to-Everything (V2X) traffic comprising V2X messages, characterized by low
         * latency, high reliability, and high availability.
         * Bits: 1 0 1 0 0 1 0 1
         */
        VEHICULAR_COMMUNICATIONS = 0xA5u8,

        /**
         * Real time interactive traffic, for example, for gaming or AR/VR.
         * Bits: 1 0 1 0 0 1 1 0
         */
        REAL_TIME_INTERACTIVE = 0xA6u8,

        /**
         * Unified communications traffic, which comprise communications through a single user
         * interface at the UE, for instance instant messaging, VoIP, and video collaboration
         * through the same application.
         * Bits: 1 0 1 0 0 1 1 1
         */
        UNIFIED_COMMUNICATIONS = 0xA7u8,

        /**
         * Any traffic that is not time-sensitive, e.g., firmware/software updates over the air.
         * This traffic has no critical requirements from latency or data rates perspective. This
         * traffic should/can be subject of scheduling (e.g., at specific time of day) by the
         * applications/networks.
         * Bits: 1 0 1 0 1 0 0 0
         */
        BACKGROUND = 0xA8u8,

        /**
         * Mission-critical communications, may include MC-PTT, MC video, and MC data.
         * Bits: 1 0 1 0 1 0 0 1
         */
        MISSION_CRITICAL_COMMUNICATIONS = 0xA9u8,

        /**
         * Time Critical Communications, with bounded, low to very low latency requirements, and
         * high availability.
         * Bits: 1 0 1 0 1 0 1 0
         */
        TIME_CRITICAL_COMMUNICATIONS = 0xAAu8,

        /**
         * Traffic which has low latency requirements and is tolerant to some loss, hence using
         * un-acknowledged mode at the Radio Link Control (RLC) layer. E.g., for certain real
         * time voice or video traffic.
         * Bits: 1 0 1 0 1 0 1 1
         */
        LOW_LATENCY_LOSS_TOLERANT_UNACK = 0xABu8,
    }

    /**
     * Connection capability defined in 3GPP TS 124.526 table 5.2.1
     */
    ConnectionCapability connectionCapability = ConnectionCapability.UNKNOWN;
}
