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

package android.hardware.tv.mediaquality;

import android.hardware.tv.mediaquality.DigitalOutput;
import android.hardware.tv.mediaquality.DolbyAudioProcessing;
import android.hardware.tv.mediaquality.DownmixMode;
import android.hardware.tv.mediaquality.DtsVirtualX;
import android.hardware.tv.mediaquality.EqualizerDetail;
import android.hardware.tv.mediaquality.QualityLevel;
import android.hardware.tv.mediaquality.SoundStyle;

/**
 * The parameters for Sound Profile.
 */
@VintfStability
union SoundParameter {
    /*
     * This parameter controls the balance between the left and tight speakers.
     * The valid range is -50 to 50, where:
     *   - Negative values shift the balance towards the left speaker.
     *   - Positive values shift the balance towards the right speaker.
     *   - 0 represents a balanced output.
     */
    int balance;

    /*
     * Bass controls the intensity of low-frequency sounds.
     * The valid range is 0 - 100.
     */
    int bass;

    /*
     * Treble controls the intensity of high-frequency sounds.
     * The valid range is 0 - 100.
     */
    int treble;

    /* Enable surround sound. */
    boolean surroundSoundEnabled;

    /*
     * Equalizer can fine-tune the audio output by adjusting the loudness of different
     * frequency bands;
     * The frequency bands are 120Hz, 500Hz, 1.5kHz, 5kHz, 10kHz.
     * Each band have a value of -50 to 50.
     */
    EqualizerDetail equalizerDetail;

    /* Enable speaker output. */
    boolean speakersEnabled;

    /* Speaker delay in ms. */
    int speakersDelayMs;

    /* eARC allows for higher bandwidth audio transmission over HDMI */
    boolean enhancedAudioReturnChannelEnabled;

    /* Enable auto volume control sound effect. */
    boolean autoVolumeControl;

    /* Enable downmix mode. */
    DownmixMode downmixMode;

    /* Enable dynamic range compression */
    boolean dtsDrc;

    /* Sound effects from Dobly */
    @nullable DolbyAudioProcessing dolbyAudioProcessing;

    /* Sound effect from Dolby. */
    QualityLevel dolbyDialogueEnhancer;

    /* Sound effect from DTS. */
    @nullable DtsVirtualX dtsVirtualX;

    /* Digital output mode. */
    DigitalOutput digitalOutput;

    /* Digital output delay in ms. */
    int digitalOutputDelayMs;

    /**
     * Determines whether the current profile is actively in use or not.
     */
    boolean activeProfile;

    /*
     * Sound style of the profile.
     *
     * The default value is user customized profile.
     */
    SoundStyle soundStyle;

    /**
     * Adjusts the left/right audio balance for the built-in speakers.
     * The range is -50 (left) to 50 (right), with 0 being centered.
     */
    int balanceSpeaker;

    /**
     * Adjusts the left/right audio balance for a connected Bluetooth device.
     * The range is -50 (left) to 50 (right), with 0 being centered.
     */
    int balanceBluetooth;

    /**
     * Adjusts the left/right audio balance for a connected headphone.
     * The range is -50 (left) to 50 (right), with 0 being centered.
     */
    int balanceHeadphone;

    /**
     * Toggles the High-Resolution Audio path, which offers better-than-CD quality
     * playback with higher sampling rates and/or bit depth.
     */
    boolean hiResAudio;

    /**
     * Reports the audio latency of the connected Bluetooth device in microseconds.
     * This value can be used by A/V sync logic to maintain lip-sync.
     */
    int btLatencyUs;

    /**
     * Controls the output routing of the Audio Description track to the internal speakers.
     * <p>If set to {@code true}, the AD track will be mixed with the main audio
     * and played through the device's built-in speakers.</p>
     * <p><b>Dependency:</b> This setting is ignored if the device does not have
     * internal speakers or if audio routing is forcibly overridden by system policy.</p>
     */
    boolean adSpeakerEnable;

    /**
     * Controls the output routing of the Audio Description track to connected headphones.
     * <p>If set to {@code true}, the AD track will be mixed and played through
     * wired or Bluetooth headsets.</p>
     * <p><b>Note:</b> This enables independent consumption of AD content if the
     * audio engine supports dual-routing (e.g., AD on headphones, Main Audio on speakers).</p>
     */
    boolean adHeadphoneEnable;

    /**
     * Sets the relative volume gain for the Audio Description track.
     *
     * <p><b>Unit:</b> Integer Percentage (0-100)</p>
     * <p><b>Default:</b> Typically defaults to 50 or the system-wide accessibility volume
     * preference.</p>
     *
     * <p>This value controls the mixing level of the secondary audio stream (AD)
     * before it is combined with the main program audio.
     * <ul>
     * <li>{@code 0}: AD track is muted.</li>
     * <li>{@code 100}: AD track is at maximum mixing volume.</li>
     * </ul>
     * </p>
     *
     * @param volume An integer between 0 and 100.
     */
    int adVolume;

    /**
     * Enables automatic Pan and Fade (Ducking) behavior for the main audio.
     * <p>When set to {@code true}, the audio engine will apply standard broadcast mixing rules:
     * <ul>
     * <li><b>Fade:</b> The main program audio volume is lowered ("ducked") when
     * audio description is present to ensure the narrator is intelligible.</li>
     * <li><b>Pan:</b> The main audio may be spatially shifted (e.g., to background channels)
     * to center the audio description track.</li>
     * </ul>
     * </p>
     * <p>If set to {@code false}, the AD track is mixed simply as an overlay without
     * modifying the volume or position of the main audio track.</p>
     */
    boolean panFadeEnable;
}
