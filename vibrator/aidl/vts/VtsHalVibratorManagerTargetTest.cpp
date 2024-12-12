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
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/vibrator/BnVibratorCallback.h>
#include <aidl/android/hardware/vibrator/IVibrator.h>
#include <aidl/android/hardware/vibrator/IVibratorManager.h>

#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <cmath>
#include <future>

#include "test_utils.h"

using aidl::android::hardware::vibrator::BnVibratorCallback;
using aidl::android::hardware::vibrator::CompositeEffect;
using aidl::android::hardware::vibrator::CompositePrimitive;
using aidl::android::hardware::vibrator::Effect;
using aidl::android::hardware::vibrator::EffectStrength;
using aidl::android::hardware::vibrator::IVibrator;
using aidl::android::hardware::vibrator::IVibratorManager;
using std::chrono::high_resolution_clock;

const std::vector<Effect> kEffects{ndk::enum_range<Effect>().begin(),
                                   ndk::enum_range<Effect>().end()};
const std::vector<EffectStrength> kEffectStrengths{ndk::enum_range<EffectStrength>().begin(),
                                                   ndk::enum_range<EffectStrength>().end()};
const std::vector<CompositePrimitive> kPrimitives{ndk::enum_range<CompositePrimitive>().begin(),
                                                  ndk::enum_range<CompositePrimitive>().end()};

class CompletionCallback : public BnVibratorCallback {
  public:
    CompletionCallback(const std::function<void()>& callback) : mCallback(callback) {}
    ndk::ScopedAStatus onComplete() override {
        mCallback();
        return ndk::ScopedAStatus::ok();
    }

  private:
    std::function<void()> mCallback;
};

class VibratorAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        auto serviceName = GetParam().c_str();
        manager = IVibratorManager::fromBinder(
                ndk::SpAIBinder(AServiceManager_waitForService(serviceName)));
        ASSERT_NE(manager, nullptr);
        EXPECT_OK(manager->getCapabilities(&capabilities));
        EXPECT_OK(manager->getVibratorIds(&vibratorIds));
    }

    std::shared_ptr<IVibratorManager> manager;
    int32_t capabilities;
    std::vector<int32_t> vibratorIds;
};

TEST_P(VibratorAidl, ValidateExistingVibrators) {
    std::shared_ptr<IVibrator> vibrator;
    for (int32_t id : vibratorIds) {
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);
    }
}

TEST_P(VibratorAidl, GetVibratorWithInvalidId) {
    int32_t invalidId = *max_element(vibratorIds.begin(), vibratorIds.end()) + 1;
    std::shared_ptr<IVibrator> vibrator;
    EXPECT_ILLEGAL_ARGUMENT(manager->getVibrator(invalidId, &vibrator));
    ASSERT_EQ(vibrator, nullptr);
}

TEST_P(VibratorAidl, ValidatePrepareSyncedExistingVibrators) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (vibratorIds.empty()) return;
    EXPECT_OK(manager->prepareSynced(vibratorIds));
    EXPECT_OK(manager->cancelSynced());
}

TEST_P(VibratorAidl, PrepareSyncedEmptySetIsInvalid) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    std::vector<int32_t> emptyIds;
    EXPECT_ILLEGAL_ARGUMENT(manager->prepareSynced(emptyIds));
}

TEST_P(VibratorAidl, PrepareSyncedNotSupported) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) {
        EXPECT_UNKNOWN_OR_UNSUPPORTED(manager->prepareSynced(vibratorIds));
    }
}

TEST_P(VibratorAidl, PrepareOnNotSupported) {
    if (vibratorIds.empty()) return;
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_PREPARE_ON)) {
        uint32_t durationMs = 250;
        EXPECT_OK(manager->prepareSynced(vibratorIds));
        std::shared_ptr<IVibrator> vibrator;
        for (int32_t id : vibratorIds) {
            EXPECT_OK(manager->getVibrator(id, &vibrator));
            ASSERT_NE(vibrator, nullptr);
            EXPECT_UNKNOWN_OR_UNSUPPORTED(vibrator->on(durationMs, nullptr));
        }
        EXPECT_OK(manager->cancelSynced());
    }
}

TEST_P(VibratorAidl, PreparePerformNotSupported) {
    if (vibratorIds.empty()) return;
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_PREPARE_ON)) {
        EXPECT_OK(manager->prepareSynced(vibratorIds));
        std::shared_ptr<IVibrator> vibrator;
        for (int32_t id : vibratorIds) {
            EXPECT_OK(manager->getVibrator(id, &vibrator));
            ASSERT_NE(vibrator, nullptr);
            int32_t lengthMs = 0;
            EXPECT_UNKNOWN_OR_UNSUPPORTED(
                    vibrator->perform(kEffects[0], kEffectStrengths[0], nullptr, &lengthMs));
        }
        EXPECT_OK(manager->cancelSynced());
    }
}

TEST_P(VibratorAidl, PrepareComposeNotSupported) {
    if (vibratorIds.empty()) return;
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_PREPARE_ON)) {
        std::vector<CompositeEffect> composite;
        CompositeEffect effect;
        effect.delayMs = 10;
        effect.primitive = kPrimitives[0];
        effect.scale = 1.0f;
        composite.emplace_back(effect);

        EXPECT_OK(manager->prepareSynced(vibratorIds));
        std::shared_ptr<IVibrator> vibrator;
        for (int32_t id : vibratorIds) {
            EXPECT_OK(manager->getVibrator(id, &vibrator));
            ASSERT_NE(vibrator, nullptr);
            EXPECT_UNKNOWN_OR_UNSUPPORTED(vibrator->compose(composite, nullptr));
        }
        EXPECT_OK(manager->cancelSynced());
    }
}

TEST_P(VibratorAidl, TriggerWithCallback) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_PREPARE_ON)) return;
    if (!(capabilities & IVibratorManager::CAP_TRIGGER_CALLBACK)) return;
    if (vibratorIds.empty()) return;

    std::promise<void> completionPromise;
    std::future<void> completionFuture{completionPromise.get_future()};
    auto callback = ndk::SharedRefBase::make<CompletionCallback>(
            [&completionPromise] { completionPromise.set_value(); });
    uint32_t durationMs = 250;
    std::chrono::milliseconds timeout{durationMs * 2};

    EXPECT_OK(manager->prepareSynced(vibratorIds));
    std::shared_ptr<IVibrator> vibrator;
    for (int32_t id : vibratorIds) {
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);
        EXPECT_OK(vibrator->on(durationMs, nullptr));
    }

    EXPECT_OK(manager->triggerSynced(callback));
    EXPECT_EQ(completionFuture.wait_for(timeout), std::future_status::ready);
    EXPECT_OK(manager->cancelSynced());
}

TEST_P(VibratorAidl, TriggerSyncNotSupported) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) {
        EXPECT_UNKNOWN_OR_UNSUPPORTED(manager->triggerSynced(nullptr));
    }
}

TEST_P(VibratorAidl, TriggerCallbackNotSupported) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_TRIGGER_CALLBACK)) {
        auto callback = ndk::SharedRefBase::make<CompletionCallback>([] {});
        EXPECT_OK(manager->prepareSynced(vibratorIds));
        EXPECT_UNKNOWN_OR_UNSUPPORTED(manager->triggerSynced(callback));
        EXPECT_OK(manager->cancelSynced());
    }
}

std::vector<std::string> FindVibratorManagerNames() {
    std::vector<std::string> names;
    constexpr auto callback = [](const char* instance, void* context) {
        std::string fullName = std::string(IVibratorManager::descriptor) + "/" + instance;
        static_cast<std::vector<std::string>*>(context)->emplace_back(fullName);
    };
    AServiceManager_forEachDeclaredInstance(IVibratorManager::descriptor,
                                            static_cast<void*>(&names), callback);
    return names;
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VibratorAidl);
INSTANTIATE_TEST_SUITE_P(Vibrator, VibratorAidl, testing::ValuesIn(FindVibratorManagerNames()),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
