/*
 * Copyright 2025 The Android Open Source Project
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

package android.hardware.bluetooth.audio;

import android.hardware.bluetooth.audio.ChannelMode;
import android.hardware.bluetooth.audio.CodecSpecificCapabilitiesLtv;
import android.hardware.bluetooth.audio.MetadataLtv;

/**
 * Used to specify the capabilities of the codecs supported by the offloader.
 * This describes the peripheral capabilities that can be exposed in the Published Audio
 * Capabilities Service (PACS), allowing the use of structures defined in the
 * Bluetooth SIG Assigned Numbers.
 */
@VintfStability
parcelable LeAudioPeripheralCapabilities {
    @VintfStability
    parcelable CodecCapabilities {
        /*
         * PCM is Input for encoder, Output for decoder
         */
        byte[] pcmBitDepth;
        /*
         * codec-specific parameters
         */
        int[] samplingFrequencyHz;
        /*
         * FrameDuration based on microseconds.
         */
        int[] frameDurationUs;
        /*
         * minimum length in octets of a codec frame
         */
        int[] minOctectsPerFrame;
        /*
         * maximum length in octets of a codec frame
         */
        int[] maxOctectsPerFrame;
        /*
         * Number of blocks of codec frames per single SDU (Service Data Unit)
         */
        byte[] blocksPerSdu;
        /*
         * Channel mode used in A2DP special audio, ignored in standard LE Audio mode
         */
        ChannelMode[] channelMode;
    }
    /**
     * Used to specify the capabilities of the codecs supported by Hardware Encoding.
     */
    @VintfStability
    parcelable VendorCodecCapabilities {
        ParcelableHolder extension;
        byte[] vendorCodecSpecificCapabilities;
    }

    /**
     * Defines the supported audio codec capabilities based on the Bluetooth LE Audio
     * specifications.
     */
    CodecCapabilities codecCapabilities;

    /**
     * Optional vendor-specific capabilities, typically used to provide
     * additional or proprietary codec information, especially for hardware encoding.
     * Can be null if no vendor-specific capabilities are present.
     */
    @nullable VendorCodecCapabilities vendorCapabilities;

    /**
     * Metadata to be used by the stack in the PAC record.
     * Bluetooth stack will group capabilities in the PAC record which are identified by the same
     * metadata.
     */
    @nullable MetadataLtv[] metadata;
}
