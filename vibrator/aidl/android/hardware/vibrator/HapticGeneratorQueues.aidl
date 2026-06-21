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

import android.hardware.common.fmq.MQDescriptor;
import android.hardware.common.fmq.SynchronizedReadWrite;
import android.hardware.vibrator.HapticGeneratorCommand;
import android.hardware.vibrator.HapticGeneratorReply;
import android.hardware.vibrator.VibrationEffectContent;

/**
 * A collection of message queues for haptic PCM data generation.
 *
 * The following sequence of operations is used for generating haptic PCM data:
 *  1. The framework writes a 'startEffect' command to the 'command' queue and waits for the reply.
 *     If needed, the framework will also clear the 'pcm' queue.
 *  2. The HAL clears the 'effect' queue, if needed, and prepares to generate PCM
 *     data for a new effect.
 *  3. The HAL writes a reply into the 'reply' queue informing it's ready to
 *     start generation.
 *  4. If there is more VibrationEffectContent to be sent to the HAL,
 *     the framework writes VibrationEffectContent into the 'effect' queue.
 *  5. [Optional] If the entire effect was written into the 'effect' queue in
 *     the previous step:
 *     5.1 The framework sends a 'completeEffect' command to the 'command'
 *         queue and waits for reply.
 *     5.2 The HAL writes a reply into the 'reply' queue acknowledging.
 *  6. The framework writes a 'burstBytes' command into the 'command' queue and
 *     waits for the reply.
 *  7. The HAL reads any available VibrationEffectContent from the 'effect' queue.
 *  8. The HAL generates the next bytes of haptic PCM data for the current
 *     effect and writes it into the 'pcm' queue. The generated data is not
 *     necessarily based on the most recent data read from the 'effect' queue.
 *  9. The HAL writes a reply into the 'reply' queue informing how many bytes
 *     were written in the previous step.
 * 10. The framework reads all PCM bytes from the 'pcm' queue.
 * 11. The framework determines if the effect is complete. The effect is
 *     considered complete if the 'completeEffect' command has been acknowledged
 *     by the HAL and the HAL has replied to a 'burstBytes' command with 0
 *     bytes ready.
 * 12. Go back to step 4 until generation is complete.
 */
@VintfStability
parcelable HapticGeneratorQueues {
    /**
     * Id of the vibrator associated with these haptic generator queues.
     */
    int vibratorId;

    /**
     * For commands from the framework to the HAL (e.g., burst, close).
     */
    MQDescriptor<HapticGeneratorCommand, SynchronizedReadWrite> command;

    /**
     * For vibration data from the framework to the HAL.
     * This queue acts as a buffer for the vibration effect data that the HAL
     * will process to generate haptic PCM.
     */
    MQDescriptor<VibrationEffectContent, SynchronizedReadWrite> effect;

    /**
     * For replies from the HAL back to the framework.
     */
    MQDescriptor<HapticGeneratorReply, SynchronizedReadWrite> reply;

    /**
     * For the generated haptic PCM data from the HAL to the framework.
     * This queue provides the raw haptic waveform, which can be played through
     * the audio pipeline to potentially achieve precise audio-haptic synchronization.
     */
    MQDescriptor<byte, SynchronizedReadWrite> pcm;
}
