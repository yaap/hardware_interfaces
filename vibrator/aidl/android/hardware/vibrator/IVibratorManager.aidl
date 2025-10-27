/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.vibrator;

import android.hardware.vibrator.HapticGeneratorConfig;
import android.hardware.vibrator.HapticGeneratorSession;
import android.hardware.vibrator.IVibrationSession;
import android.hardware.vibrator.IVibrator;
import android.hardware.vibrator.IVibratorCallback;
import android.hardware.vibrator.VibrationSessionConfig;

@VintfStability
interface IVibratorManager {
    /**
     * Whether prepare/trigger synced are supported.
     */
    const int CAP_SYNC = 1 << 0;
    /**
     * Whether IVibrator 'on' can be used with 'prepareSynced' function.
     */
    const int CAP_PREPARE_ON = 1 << 1;
    /**
     * Whether IVibrator 'perform' can be used with 'prepareSynced' function.
     */
    const int CAP_PREPARE_PERFORM = 1 << 2;
    /**
     * Whether IVibrator 'compose' can be used with 'prepareSynced' function.
     */
    const int CAP_PREPARE_COMPOSE = 1 << 3;
    /**
     * Whether IVibrator 'on' can be triggered with other functions in sync with 'triggerSynced'.
     */
    const int CAP_MIXED_TRIGGER_ON = 1 << 4;
    /**
     * Whether IVibrator 'perform' can be triggered with other functions in sync with
     * 'triggerSynced'.
     */
    const int CAP_MIXED_TRIGGER_PERFORM = 1 << 5;
    /**
     * Whether IVibrator 'compose' can be triggered with other functions in sync with
     * 'triggerSynced'.
     */
    const int CAP_MIXED_TRIGGER_COMPOSE = 1 << 6;
    /**
     * Whether on w/ IVibratorCallback can be used w/ 'trigerSynced' function.
     */
    const int CAP_TRIGGER_CALLBACK = 1 << 7;
    /**
     * Whether vibration sessions are supported.
     */
    const int CAP_START_SESSIONS = 1 << 8;

    /**
     * Whether haptic generator is supported.
     */
    const int CAP_HAPTIC_GENERATOR = 1 << 9;

    /**
     * Determine capabilities of the vibrator manager HAL (CAP_* mask)
     */
    int getCapabilities();

    /**
     * List the id of available vibrators. This result should be static and not change.
     */
    int[] getVibratorIds();

    /**
     * Return an available vibrator identified with given id.
     */
    IVibrator getVibrator(in int vibratorId);

    /**
     * Start preparation for a synced vibration
     *
     * This function must only be called after the previous synced vibration was triggered or
     * canceled (through cancelSynced()).
     *
     * Doing this operation while any of the specified vibrators is already on is undefined
     * behavior. Clients should explicitly call off in each vibrator.
     *
     * @param vibratorIds ids of the vibrators to play vibrations in sync.
     */
    void prepareSynced(in int[] vibratorIds);

    /**
     * Trigger a prepared synced vibration
     *
     * Trigger a previously-started preparation for synced vibration, if any.
     * A callback is only expected to be supported when getCapabilities CAP_TRIGGER_CALLBACK
     * is specified.
     *
     * @param callback A callback used to inform Frameworks of state change, if supported.
     */
    void triggerSynced(in @nullable IVibratorCallback callback);

    /**
     * Cancel preparation of synced vibration
     *
     * Cancel a previously-started preparation for synced vibration, if any.
     */
    void cancelSynced();

    /**
     * Start a vibration session.
     *
     * A vibration session can be used to send commands without resetting the vibrator state. Once a
     * session starts, the individual vibrators can receive one or more commands like on(),
     * performEffect(), setAmplitude(), etc. The vibrations performed in a session must have the
     * same behavior they have outside them. Multiple commands can be synced in a session via
     * prepareSynced as usual.
     *
     * Starting a session on a vibrator already in another session or in a prepareSynced state is
     * not allowed and should throw illegal state. The end of a session should always notify the
     * callback provided, even if it ends prematurely due to an error.
     *
     * This may not be supported and this support is reflected in
     * getCapabilities (CAP_START_SESSIONS). IVibratorCallback.onComplete() support is required for
     * this API.
     *
     * @param vibratorIds ids of the vibrators in the session.
     * @param config The parameters for starting a vibration session.
     * @param callback A callback used to inform Frameworks of state change.
     * @throws :
     *         - EX_UNSUPPORTED_OPERATION if unsupported, as reflected by getCapabilities.
     *         - EX_ILLEGAL_ARGUMENT for invalid vibrator IDs.
     *         - EX_ILLEGAL_STATE for vibrator IDs already in a session or in a prepareSynced state.
     *         - EX_SERVICE_SPECIFIC for bad vendor data.
     */
    IVibrationSession startSession(in int[] vibratorIds, in VibrationSessionConfig config,
            in @nullable IVibratorCallback callback);

    /**
     * Aborts and clears all ongoing vibration and haptic generator sessions.
     *
     * <p>This can be used to reset the vibrator manager and its vibrators to an
     * idle state. Any active {@link IVibrationSession} or {@link HapticGeneratorSession} will
     * be terminated, and their respective {@link IVibratorCallback#onComplete} callbacks
     * will be triggered.
     */
    void clearSessions();

    /**
     * Starts a new haptic generator session that converts effects to haptic PCM data.
     *
     * <p>A haptic generator session can be used to convert a stream of
     * {@link VibrationEffectContent} into a haptic PCM data stream.
     *
     * <p>The session operates independently and can run concurrently with
     * vibrations being played via {@link IVibrator}, ensuring that PCM generation does not
     * block other haptic functionality. The same vibrator can have multiple generator sessions
     * running in parallel.
     *
     * <p>Communication is managed through a set of Fast Message Queues (FMQs) which are returned in
     * the {@link HapticGeneratorSession} object. The framework uses these queues to:
     * <ol>
     * <li> Send commands (e.g., burst command, close session). </li>
     * <li> Stream the {@link VibrationEffectContent} to the HAL. </li>
     * <li> Receive status replies from the HAL.</li>
     * <li> Read the generated haptic PCM data from the HAL.</li>
     * </ol>
     *
     * <p>The provided callback will be triggered when the session ends for any reason, such as
     * being terminated by a call to `IVibratorManager.clearSessions()`, a `close` command, or an
     * error within the HAL.
     *
     * <p>This may not be supported, which is reflected in getCapabilities() (CAP_HAPTIC_GENERATOR).
     *
     * @param vibratorIds ids of the vibrators in the session.
     * @param config Configuration parameters for the PCM generation.
     * @param callback A callback used to inform Frameworks of state changes.
     * @return A {@link HapticGeneratorSession} containing the communication queues.
     * @throws :
     *         - EX_UNSUPPORTED_OPERATION if unsupported, as reflected by getCapabilities.
     *         - EX_ILLEGAL_ARGUMENT for invalid vibrator IDs or invalid config data.
     */
    HapticGeneratorSession startHapticGeneratorSession(in int[] vibratorIds,
            in HapticGeneratorConfig config, in @nullable IVibratorCallback callback);
}
