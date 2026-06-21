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
 * See the License for the specific language governing permIssions and
 * limitations under the License.
 */

#pragma once

#include <cstdint>
#include <vector>

class NfcProprietaryCaps {
  public:
    enum class PassiveObserveMode {
        NOT_SUPPORTED = 0,
        SUPPORT_WITH_RF_DEACTIVATION = 1,
        SUPPORT_WITHOUT_RF_DEACTIVATION = 2,
    };

    explicit NfcProprietaryCaps(const std::vector<uint8_t>& caps);

    PassiveObserveMode getPassiveObserveMode() const { return mPassiveObserveMode; }
    bool isPollingFrameNotificationSupported() const {
        return mIsPollingFrameNotificationSupported;
    }
    bool isPowerSavingModeSupported() const { return mIsPowerSavingModeSupported; }
    bool isAutotransactPollingLoopFilterSupported() const {
        return mIsAutotransactPollingLoopFilterSupported;
    }
    int getNumberOfExitFramesSupported() const { return mNumberOfExitFramesSupported; }
    bool isReaderModeAnnotationSupported() const { return mIsReaderModeAnnotationSupported; }

  private:
    static constexpr int PASSIVE_OBSERVE_MODE = 0;
    static constexpr int POLLING_FRAME_NTF = 1;
    static constexpr int POWER_SAVING_MODE = 2;
    static constexpr int AUTOTRANSACT_POLLING_LOOP_FILTER = 3;
    static constexpr int NUMBER_OF_EXIT_FRAMES_SUPPORTED = 4;
    static constexpr int READER_MODE_ANNOTATIONS_SUPPORTED = 5;

    PassiveObserveMode mPassiveObserveMode = PassiveObserveMode::NOT_SUPPORTED;
    bool mIsPollingFrameNotificationSupported = false;
    bool mIsPowerSavingModeSupported = false;
    bool mIsAutotransactPollingLoopFilterSupported = false;
    int mNumberOfExitFramesSupported = 0;
    bool mIsReaderModeAnnotationSupported = false;
};
