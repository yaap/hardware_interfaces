/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include "impl/RadioConfig.h"
#include "impl/RadioData.h"
#include "impl/RadioModem.h"
#include "impl/RadioNetwork.h"
#include "impl/RadioSim.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

namespace android::hardware::radio::service {

using namespace std::string_literals;

static std::vector<std::shared_ptr<ndk::ICInterface>> gPublishedHals;

static void publishRadioConfig() {
    auto aidlHal = ndk::SharedRefBase::make<RadioConfig>();
    gPublishedHals.push_back(aidlHal);
    const auto instance = RadioConfig::descriptor + "/default"s;
    const auto status = AServiceManager_addService(aidlHal->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK);
}

template <typename T>
static void publishRadioHal(const std::string& slot,
                            std::shared_ptr<minimal::SlotContext> context) {
    const auto instance = T::descriptor + "/"s + slot;
    if (!AServiceManager_isDeclared(instance.c_str())) {
        LOG(INFO) << instance << " is not declared in VINTF (this may be intentional)";
        return;
    }
    LOG(DEBUG) << "Publishing " << instance;

    auto aidlHal = ndk::SharedRefBase::make<T>(context);
    gPublishedHals.push_back(aidlHal);
    const auto status = AServiceManager_addService(aidlHal->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK);
}

void main() {
    base::InitLogging(nullptr, base::LogdLogger(base::RADIO));
    base::SetDefaultTag("minradio");
    base::SetMinimumLogSeverity(base::VERBOSE);
    LOG(DEBUG) << "Minimal Radio HAL service starting...";
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();

    auto slot1Context = std::make_shared<minimal::SlotContext>(1);

    publishRadioConfig();
    publishRadioHal<RadioData>("slot1", slot1Context);
    publishRadioHal<RadioModem>("slot1", slot1Context);
    publishRadioHal<RadioNetwork>("slot1", slot1Context);
    publishRadioHal<RadioSim>("slot1", slot1Context);

    LOG(DEBUG) << "Minimal Radio HAL service is operational";
    ABinderProcess_joinThreadPool();
    LOG(FATAL) << "Minimal Radio HAL service has stopped";
}

}  // namespace android::hardware::radio::service

int main() {
    android::hardware::radio::service::main();
    return EXIT_FAILURE;  // should not reach
}
