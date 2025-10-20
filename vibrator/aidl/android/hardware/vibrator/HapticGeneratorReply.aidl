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
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.vibrator;

@VintfStability
@FixedSize
parcelable HapticGeneratorReply {
    /**
     * One of Binder STATUS_* statuses:
     *  - STATUS_OK: the command has completed successfully.
     *  - STATUS_BAD_VALUE: invalid value in the 'Command' structure.
     *  - STATUS_INVALID_OPERATION: the command is not applicable in the
     *                              current state of the stream.
     *  - STATUS_NOT_ENOUGH_DATA: a read or write error has occurred for a
     *                            queue, or the HAL requires more effect data
     *                            to be written to the 'effect' queue before
     *                            more PCM data can be generated.
     */
    int status;

    /**
     * Number of bytes of haptic PCM now available in the
     * `HapticGeneratorQueues.pcm` queue.
     */
    int burstBytesReady;
}
