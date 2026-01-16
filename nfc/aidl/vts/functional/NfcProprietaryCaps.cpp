/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law-or-agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permIssions and
 * limitations under the License.
 */

#include "NfcProprietaryCaps.h"

#include <android-base/logging.h>

NfcProprietaryCaps::NfcProprietaryCaps(const std::vector<uint8_t>& caps) {
    int offset = 0;
    while ((offset + 2) < caps.size()) {
        int id = caps[offset++];
        int value_len = caps[offset++];
        int value_offset = offset;
        offset += value_len;

        if (value_len < 1 || offset > caps.size()) {
            break;
        }
        switch (id) {
            case PASSIVE_OBSERVE_MODE:
                switch (caps[value_offset]) {
                    case 0:
                        mPassiveObserveMode = PassiveObserveMode::NOT_SUPPORTED;
                        break;
                    case 1:
                        mPassiveObserveMode = PassiveObserveMode::SUPPORT_WITH_RF_DEACTIVATION;
                        break;
                    case 2:
                        mPassiveObserveMode = PassiveObserveMode::SUPPORT_WITHOUT_RF_DEACTIVATION;
                        break;
                }
                break;
            case POLLING_FRAME_NTF:
                mIsPollingFrameNotificationSupported = caps[value_offset] == 0x1;
                break;
            case POWER_SAVING_MODE:
                mIsPowerSavingModeSupported = caps[value_offset] == 0x1;
                break;
            case AUTOTRANSACT_POLLING_LOOP_FILTER:
                mIsAutotransactPollingLoopFilterSupported = caps[value_offset] == 0x1;
                break;
            case NUMBER_OF_EXIT_FRAMES_SUPPORTED:
                mNumberOfExitFramesSupported = caps[value_offset];
                break;
            case READER_MODE_ANNOTATIONS_SUPPORTED:
                mIsReaderModeAnnotationSupported = caps[value_offset] == 0x1;
                break;
        }
    }
}
