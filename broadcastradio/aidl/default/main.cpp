/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include "BroadcastRadio.h"
#include "VirtualRadio.h"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <chrono>
#include <cinttypes>
#include <thread>

using ::aidl::android::hardware::broadcastradio::BroadcastRadio;
using ::aidl::android::hardware::broadcastradio::VirtualRadio;

namespace {

constexpr const char* kInitDelayProperty = "persist.vendor.broadcastradio.mock.init_delay_ms";

}

void delayInitForTesting() {
    // Read a system property for the delay in milliseconds
    int64_t delay_ms = android::base::GetIntProperty<int64_t>(kInitDelayProperty, 0);
    if (delay_ms > 0) {
        LOG(INFO) << "Mock BroadcastRadio HAL delaying initialization by " << delay_ms << " ms";
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        LOG(INFO) << "Mock BroadcastRadio HAL proceeding with initialization";
    }
}

int main() {
    android::base::SetDefaultTag("BcRadioAidlDef");
    delayInitForTesting();
    ABinderProcess_setThreadPoolMaxThreadCount(4);
    ABinderProcess_startThreadPool();

    const VirtualRadio& amFmRadioMock = VirtualRadio::getAmFmRadio();
    std::shared_ptr<BroadcastRadio> broadcastRadio =
            ::ndk::SharedRefBase::make<BroadcastRadio>(amFmRadioMock);
    const std::string instanceAmFm = std::string() + BroadcastRadio::descriptor + "/amfm";
    binder_status_t statusAmFm =
            AServiceManager_addService(broadcastRadio->asBinder().get(), instanceAmFm.c_str());
    CHECK_EQ(statusAmFm, STATUS_OK)
            << "Failed to register Broadcast Radio AM/FM HAL implementation";

    const VirtualRadio& dabRadioMock = VirtualRadio::getDabRadio();
    std::shared_ptr<BroadcastRadio> dabRadio =
            ::ndk::SharedRefBase::make<BroadcastRadio>(dabRadioMock);
    const std::string instanceDab = std::string() + BroadcastRadio::descriptor + "/dab";
    binder_status_t statusDab =
            AServiceManager_addService(dabRadio->asBinder().get(), instanceDab.c_str());
    CHECK_EQ(statusDab, STATUS_OK) << "Failed to register Broadcast Radio DAB HAL implementation";

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
