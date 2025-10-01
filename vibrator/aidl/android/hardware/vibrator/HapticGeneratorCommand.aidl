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

package android.hardware.vibrator;

@VintfStability
@FixedSize
union HapticGeneratorCommand {
    /**
     * Reserved space for future additions to this union. This ensures
     * backward compatibility.
     */
    byte[32] reserved = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0};

    /**
     * A set of commands that manage the state of a single vibration effect conversion.
     */
    @Backing(type="byte")
    enum Effect {
        /**
         * Informs the HAL to prepare for a new vibration effect.
         *
         * <p>If this command is received while another effect is being processed,
         * it acts as an implicit cancellation of the previous effect.
         *
         * <p>Upon receiving this command, the HAL MUST clear the
         * HapticGeneratorQueues.effect queue and prepare to receive new data. Any
         * subsequent `burstBytes` command must only use the new effect data. The
         * client is responsible for clearing the HapticGeneratorQueues.pcm queue to
         * discard any buffered PCM data from the previous effect.
         *
         * <p>The framework will not write to the 'effect' queue until a success
         * reply is received for this command.
         */
        START,
        /**
         * Informs the HAL that the framework has sent all data for the currently
         * active effect.
         *
         * <p>The HAL must continue to service `burstBytes` commands until all
         * generated data has been consumed.
         *
         * <p>This command must be preceded by a `START` command. If this
         * command is received when no effect is active, the HAL MUST return a
         * `STATUS_INVALID_OPERATION` status in the reply.
         */
        COMPLETE,
        /**
         * Informs the HAL to immediately stop processing the currently active effect.
         *
         * <p>Upon receiving this command, the HAL MUST clear the
         * HapticGeneratorQueues.effect queue. The client is responsible for clearing
         * the HapticGeneratorQueues.pcm queue to discard any buffered PCM data from
         * the cancelled effect.
         *
         * <p>Any `burstBytes` command received before a new `START` command
         * should be rejected. If no effect is currently active, the HAL MUST reply
         * with STATUS_INVALID_OPERATION.
         */
        CANCEL,
    }
    Effect effect = Effect.START;

    /**
     * A set of commands that manage the state of the entire haptic generator session.
     */
    @Backing(type="byte")
    enum Session {
        /**
         * Informs the HAL that the session is terminated and it should release all
         * server-side resources.
         *
         * <p>All commands after this should be rejected.
         */
        CLOSE,
    }
    Session session = Session.CLOSE;

    /**
     * Informs the HAL the number of bytes of PCM data that it should generate
     * and write into the `HapticGeneratorQueues.pcm`.
     *
     * <p>The HAL should attempt to generate this many bytes, but is allowed
     * to generate fewer bytes if the vibration effect completes partway through
     * the request. The HAL must report the actual number of bytes written in the
     * 'HapticGeneratorReply.burstBytesReady' field.
     *
     * <p>If the HAL cannot generate any PCM data because it is waiting for more
     * effect input data from the framework, it MUST reply with a
     * `STATUS_NOT_ENOUGH_DATA` status.
     *
     * <p>This command must be preceded by a `START` command. If this
     * command is received when no effect is active, the HAL MUST return a
     * `STATUS_INVALID_OPERATION` status in the reply.
     */
    int burstBytes;
}
