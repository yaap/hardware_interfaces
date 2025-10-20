/**
 * Copyright 2025, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.graphics.composer3;

@VintfStability
parcelable ActiveConfigCommand {
    /**
     * The config id that should be made active.
     * If a transition from the currently active config to configId is
     * impossible, the command will fail and reason for failure will be
     * reported in the command result.
     */
    int configId;

    /**
     * Whether the command needs to be executed seamlessly, without a
     * noticeable visual artifact.
     * If a transition cannot be done seamlessly and seamless is required
     * the command will fail and the command result error will indicate
     * that seamless is not possible.
     *
     * If not seamless, the display mode must be updated even if there is no
     * present or validate command.
     */
    boolean seamlessRequired;
}
