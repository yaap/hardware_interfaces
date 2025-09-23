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
#include <aidl/android/hardware/vibrator/CompositeEffect.h>
#include <aidl/android/hardware/vibrator/HapticGeneratorCommand.h>
#include <aidl/android/hardware/vibrator/HapticGeneratorConfig.h>
#include <aidl/android/hardware/vibrator/HapticGeneratorReply.h>
#include <aidl/android/hardware/vibrator/HapticGeneratorSession.h>
#include <aidl/android/hardware/vibrator/IVibrationSession.h>
#include <aidl/android/hardware/vibrator/IVibrator.h>
#include <aidl/android/hardware/vibrator/IVibratorManager.h>
#include <aidl/android/hardware/vibrator/OneShotPrimitive.h>
#include <aidl/android/hardware/vibrator/PredefinedEffect.h>
#include <aidl/android/hardware/vibrator/PwleV2Primitive.h>
#include <aidl/android/hardware/vibrator/VibrationEffect.h>
#include <aidl/android/media/audio/common/AudioChannelLayout.h>
#include <aidl/android/media/audio/common/AudioConfigBase.h>
#include <aidl/android/media/audio/common/AudioFormatDescription.h>
#include <aidl/android/media/audio/common/AudioFormatType.h>
#include <aidl/android/media/audio/common/PcmType.h>
#include <fmq/AidlMessageQueue.h>

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <algorithm>
#include <future>

#include "haptic_generator_utils.h"
#include "test_utils.h"

using aidl::android::hardware::vibrator::BnVibratorCallback;
using aidl::android::hardware::vibrator::CompositeEffect;
using aidl::android::hardware::vibrator::CompositePrimitive;
using aidl::android::hardware::vibrator::Effect;
using aidl::android::hardware::vibrator::EffectStrength;
using aidl::android::hardware::vibrator::HapticGeneratorCommand;
using aidl::android::hardware::vibrator::HapticGeneratorConfig;
using aidl::android::hardware::vibrator::HapticGeneratorReply;
using aidl::android::hardware::vibrator::HapticGeneratorSession;
using aidl::android::hardware::vibrator::IVibrationSession;
using aidl::android::hardware::vibrator::IVibrator;
using aidl::android::hardware::vibrator::IVibratorManager;
using aidl::android::hardware::vibrator::OneShotPrimitive;
using aidl::android::hardware::vibrator::PredefinedEffect;
using aidl::android::hardware::vibrator::PwleV2Primitive;
using aidl::android::hardware::vibrator::VibrationEffect;
using aidl::android::hardware::vibrator::VibrationSessionConfig;
using aidl::android::hardware::vibrator::testing::hapticgenerator::HapticGeneratorUtils;
using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioConfigBase;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::PcmType;
using android::AidlMessageQueue;
using std::chrono::high_resolution_clock;

using namespace ::std::chrono_literals;
using namespace aidl::android::hardware::vibrator::testing;

namespace fmq = aidl::android::hardware::common::fmq;

// FMQ aliases
using CommandQueue = AidlMessageQueue<HapticGeneratorCommand, fmq::SynchronizedReadWrite>;
using EffectQueue = AidlMessageQueue<VibrationEffect, fmq::SynchronizedReadWrite>;
using ReplyQueue = AidlMessageQueue<HapticGeneratorReply, fmq::SynchronizedReadWrite>;
using PcmQueue = AidlMessageQueue<int8_t, fmq::SynchronizedReadWrite>;

const std::vector<Effect> kEffects{ndk::enum_range<Effect>().begin(),
                                   ndk::enum_range<Effect>().end()};
const std::vector<EffectStrength> kEffectStrengths{ndk::enum_range<EffectStrength>().begin(),
                                                   ndk::enum_range<EffectStrength>().end()};
const std::vector<CompositePrimitive> kPrimitives{ndk::enum_range<CompositePrimitive>().begin(),
                                                  ndk::enum_range<CompositePrimitive>().end()};

static constexpr int32_t VIBRATION_SESSIONS_MIN_VERSION = 3;

class CompletionCallback : public BnVibratorCallback {
  public:
    ndk::ScopedAStatus onComplete() override {
        completionPromise.set_value();
        return ndk::ScopedAStatus::ok();
    }

    std::future_status wait_for(const std::chrono::milliseconds& timeout) {
        return completionFuture.wait_for(timeout);
    }

  private:
    std::promise<void> completionPromise;
    std::future<void> completionFuture{completionPromise.get_future()};
};

class VibratorManagerAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        auto serviceName = GetParam().c_str();
        manager = IVibratorManager::fromBinder(
                ndk::SpAIBinder(AServiceManager_waitForService(serviceName)));
        ASSERT_NE(manager, nullptr);
        EXPECT_OK(manager->getCapabilities(&capabilities));
        EXPECT_OK(manager->getVibratorIds(&vibratorIds));
        EXPECT_OK(manager->getInterfaceVersion(&version));
    }

    virtual void TearDown() override {
        // Reset manager state between tests.
        if (capabilities & IVibratorManager::CAP_SYNC) {
            manager->cancelSynced();
        }
        if (capabilities & IVibratorManager::CAP_START_SESSIONS) {
            manager->clearSessions();
        }
        // Reset all managed vibrators.
        for (int32_t id : vibratorIds) {
            std::shared_ptr<IVibrator> vibrator;
            EXPECT_OK(manager->getVibrator(id, &vibrator));
            ASSERT_NE(vibrator, nullptr);
            EXPECT_OK(vibrator->off());
        }
    }

    std::shared_ptr<IVibratorManager> manager;
    std::shared_ptr<IVibrationSession> session;
    int32_t version;
    int32_t capabilities;
    std::vector<int32_t> vibratorIds;
};

TEST_P(VibratorManagerAidl, ValidateExistingVibrators) {
    std::shared_ptr<IVibrator> vibrator;
    for (int32_t id : vibratorIds) {
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);
    }
}

TEST_P(VibratorManagerAidl, GetVibratorWithInvalidId) {
    int32_t invalidId = *max_element(vibratorIds.begin(), vibratorIds.end()) + 1;
    std::shared_ptr<IVibrator> vibrator;
    EXPECT_ILLEGAL_ARGUMENT(manager->getVibrator(invalidId, &vibrator));
    ASSERT_EQ(vibrator, nullptr);
}

TEST_P(VibratorManagerAidl, ValidatePrepareSyncedExistingVibrators) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (vibratorIds.empty()) return;
    EXPECT_OK(manager->prepareSynced(vibratorIds));
    EXPECT_OK(manager->cancelSynced());
}

TEST_P(VibratorManagerAidl, PrepareSyncedEmptySetIsInvalid) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    std::vector<int32_t> emptyIds;
    EXPECT_ILLEGAL_ARGUMENT(manager->prepareSynced(emptyIds));
}

TEST_P(VibratorManagerAidl, PrepareSyncedNotSupported) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) {
        EXPECT_UNKNOWN_OR_UNSUPPORTED(manager->prepareSynced(vibratorIds));
    }
}

TEST_P(VibratorManagerAidl, PrepareOnNotSupported) {
    if (vibratorIds.empty()) return;
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_PREPARE_ON)) {
        EXPECT_OK(manager->prepareSynced(vibratorIds));
        std::shared_ptr<IVibrator> vibrator;
        for (int32_t id : vibratorIds) {
            EXPECT_OK(manager->getVibrator(id, &vibrator));
            ASSERT_NE(vibrator, nullptr);
            EXPECT_UNKNOWN_OR_UNSUPPORTED(vibrator->on(2000, nullptr));
        }
        EXPECT_OK(manager->cancelSynced());
    }
}

TEST_P(VibratorManagerAidl, PreparePerformNotSupported) {
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

TEST_P(VibratorManagerAidl, PrepareComposeNotSupported) {
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

TEST_P(VibratorManagerAidl, TriggerWithCallback) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_PREPARE_ON)) return;
    if (!(capabilities & IVibratorManager::CAP_TRIGGER_CALLBACK)) return;
    if (vibratorIds.empty()) return;

    auto callback = ndk::SharedRefBase::make<CompletionCallback>();
    int32_t durationMs = 250;
    EXPECT_OK(manager->prepareSynced(vibratorIds));

    std::shared_ptr<IVibrator> vibrator;
    for (int32_t id : vibratorIds) {
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);
        EXPECT_OK(vibrator->on(durationMs, nullptr));
    }

    auto timeout = std::chrono::milliseconds(durationMs) + VIBRATION_CALLBACK_TIMEOUT;
    EXPECT_OK(manager->triggerSynced(callback));
    EXPECT_EQ(callback->wait_for(timeout), std::future_status::ready);
    EXPECT_OK(manager->cancelSynced());
}

TEST_P(VibratorManagerAidl, TriggerSyncNotSupported) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) {
        EXPECT_UNKNOWN_OR_UNSUPPORTED(manager->triggerSynced(nullptr));
    }
}

TEST_P(VibratorManagerAidl, TriggerCallbackNotSupported) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_TRIGGER_CALLBACK)) {
        auto callback = ndk::SharedRefBase::make<CompletionCallback>();
        EXPECT_OK(manager->prepareSynced(vibratorIds));
        EXPECT_UNKNOWN_OR_UNSUPPORTED(manager->triggerSynced(callback));
        EXPECT_OK(manager->cancelSynced());
    }
}

TEST_P(VibratorManagerAidl, VibrationSessionsSupported) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;
    if (vibratorIds.empty()) return;

    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>();
    VibrationSessionConfig sessionConfig;
    EXPECT_OK(manager->startSession(vibratorIds, sessionConfig, sessionCallback, &session));
    ASSERT_NE(session, nullptr);

    int32_t durationMs = 250;
    std::vector<std::shared_ptr<CompletionCallback>> vibrationCallbacks;
    for (int32_t id : vibratorIds) {
        std::shared_ptr<IVibrator> vibrator;
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);

        auto vibrationCallback = ndk::SharedRefBase::make<CompletionCallback>();
        vibrationCallbacks.push_back(vibrationCallback);
        EXPECT_OK(vibrator->on(durationMs, vibrationCallback));
    }

    auto timeout = std::chrono::milliseconds(durationMs) + VIBRATION_CALLBACK_TIMEOUT;
    for (auto& cb : vibrationCallbacks) {
        EXPECT_EQ(cb->wait_for(timeout), std::future_status::ready);
    }

    // Session callback not triggered.
    EXPECT_EQ(sessionCallback->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::timeout);

    // Ending a session should not take long since the vibration was already completed
    EXPECT_OK(session->close());
    EXPECT_EQ(sessionCallback->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::ready);
}

TEST_P(VibratorManagerAidl, VibrationSessionInterrupted) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;
    if (vibratorIds.empty()) return;

    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>();
    VibrationSessionConfig sessionConfig;
    EXPECT_OK(manager->startSession(vibratorIds, sessionConfig, sessionCallback, &session));
    ASSERT_NE(session, nullptr);

    std::vector<std::shared_ptr<CompletionCallback>> vibrationCallbacks;
    for (int32_t id : vibratorIds) {
        std::shared_ptr<IVibrator> vibrator;
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);

        auto vibrationCallback = ndk::SharedRefBase::make<CompletionCallback>();
        vibrationCallbacks.push_back(vibrationCallback);
        // Vibration longer than test timeout.
        EXPECT_OK(vibrator->on(2000, vibrationCallback));
    }

    // Session callback not triggered.
    EXPECT_EQ(sessionCallback->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::timeout);

    // Interrupt vibrations and session.
    EXPECT_OK(session->abort());

    // Both callbacks triggered.
    EXPECT_EQ(sessionCallback->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::ready);
    for (auto& cb : vibrationCallbacks) {
        EXPECT_EQ(cb->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::ready);
    }
}

TEST_P(VibratorManagerAidl, VibrationSessionEndingInterrupted) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;
    if (vibratorIds.empty()) return;

    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>();
    VibrationSessionConfig sessionConfig;
    EXPECT_OK(manager->startSession(vibratorIds, sessionConfig, sessionCallback, &session));
    ASSERT_NE(session, nullptr);

    std::vector<std::shared_ptr<CompletionCallback>> vibrationCallbacks;
    for (int32_t id : vibratorIds) {
        std::shared_ptr<IVibrator> vibrator;
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);

        auto vibrationCallback = ndk::SharedRefBase::make<CompletionCallback>();
        vibrationCallbacks.push_back(vibrationCallback);
        // Vibration longer than test timeout.
        EXPECT_OK(vibrator->on(2000, vibrationCallback));
    }

    // Session callback not triggered.
    EXPECT_EQ(sessionCallback->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::timeout);

    // End session, this might take a while
    EXPECT_OK(session->close());

    // Interrupt ending session.
    EXPECT_OK(session->abort());

    // Both callbacks triggered.
    EXPECT_EQ(sessionCallback->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::ready);
    for (auto& cb : vibrationCallbacks) {
        EXPECT_EQ(cb->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::ready);
    }
}

TEST_P(VibratorManagerAidl, VibrationSessionCleared) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;
    if (vibratorIds.empty()) return;

    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>();
    VibrationSessionConfig sessionConfig;
    EXPECT_OK(manager->startSession(vibratorIds, sessionConfig, sessionCallback, &session));
    ASSERT_NE(session, nullptr);

    std::vector<std::shared_ptr<CompletionCallback>> vibrationCallbacks;
    for (int32_t id : vibratorIds) {
        std::shared_ptr<IVibrator> vibrator;
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);

        auto vibrationCallback = ndk::SharedRefBase::make<CompletionCallback>();
        vibrationCallbacks.push_back(vibrationCallback);
        EXPECT_OK(vibrator->on(2000, vibrationCallback));
    }

    // Session callback not triggered.
    EXPECT_EQ(sessionCallback->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::timeout);

    // Clearing sessions should abort ongoing session
    EXPECT_OK(manager->clearSessions());

    EXPECT_EQ(sessionCallback->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::ready);
    for (auto& cb : vibrationCallbacks) {
        EXPECT_EQ(cb->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::ready);
    }
}

TEST_P(VibratorManagerAidl, VibrationSessionsClearedWithoutSession) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;

    EXPECT_OK(manager->clearSessions());
}

TEST_P(VibratorManagerAidl, VibrationSessionsWithSyncedVibrations) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_PREPARE_ON)) return;
    if (!(capabilities & IVibratorManager::CAP_TRIGGER_CALLBACK)) return;
    if (vibratorIds.empty()) return;

    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>();
    VibrationSessionConfig sessionConfig;
    EXPECT_OK(manager->startSession(vibratorIds, sessionConfig, sessionCallback, &session));
    ASSERT_NE(session, nullptr);

    EXPECT_OK(manager->prepareSynced(vibratorIds));

    int32_t durationMs = 250;
    std::vector<std::shared_ptr<CompletionCallback>> vibrationCallbacks;
    for (int32_t id : vibratorIds) {
        std::shared_ptr<IVibrator> vibrator;
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);

        auto vibrationCallback = ndk::SharedRefBase::make<CompletionCallback>();
        vibrationCallbacks.push_back(vibrationCallback);
        EXPECT_OK(vibrator->on(durationMs, vibrationCallback));
    }

    auto triggerCallback = ndk::SharedRefBase::make<CompletionCallback>();
    EXPECT_OK(manager->triggerSynced(triggerCallback));

    auto timeout = std::chrono::milliseconds(durationMs) + VIBRATION_CALLBACK_TIMEOUT;
    EXPECT_EQ(triggerCallback->wait_for(timeout), std::future_status::ready);
    for (auto& cb : vibrationCallbacks) {
        EXPECT_EQ(cb->wait_for(timeout), std::future_status::ready);
    }

    // Session callback not triggered.
    EXPECT_EQ(sessionCallback->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::timeout);

    // Ending a session should not take long since the vibration was already completed
    EXPECT_OK(session->close());
    EXPECT_EQ(sessionCallback->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::ready);
}

TEST_P(VibratorManagerAidl, VibrationSessionWithMultipleIndependentVibrations) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;
    if (vibratorIds.empty()) return;

    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>();
    VibrationSessionConfig sessionConfig;
    EXPECT_OK(manager->startSession(vibratorIds, sessionConfig, sessionCallback, &session));
    ASSERT_NE(session, nullptr);

    for (int32_t id : vibratorIds) {
        std::shared_ptr<IVibrator> vibrator;
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);

        EXPECT_OK(vibrator->on(100, nullptr));
        EXPECT_OK(vibrator->on(200, nullptr));
        EXPECT_OK(vibrator->on(300, nullptr));
    }

    // Session callback not triggered.
    EXPECT_EQ(sessionCallback->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::timeout);

    EXPECT_OK(session->close());

    auto timeout = std::chrono::milliseconds(100 + 200 + 300) + VIBRATION_CALLBACK_TIMEOUT;
    EXPECT_EQ(sessionCallback->wait_for(timeout), std::future_status::ready);
}

TEST_P(VibratorManagerAidl, VibrationSessionsIgnoresSecondSessionWhenFirstIsOngoing) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;
    if (vibratorIds.empty()) return;

    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>();
    VibrationSessionConfig sessionConfig;
    EXPECT_OK(manager->startSession(vibratorIds, sessionConfig, sessionCallback, &session));
    ASSERT_NE(session, nullptr);

    std::shared_ptr<IVibrationSession> secondSession;
    EXPECT_ILLEGAL_STATE(
            manager->startSession(vibratorIds, sessionConfig, nullptr, &secondSession));
    EXPECT_EQ(secondSession, nullptr);

    // First session was not cancelled.
    EXPECT_EQ(sessionCallback->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::timeout);

    // First session still ongoing, we can still vibrate.
    int32_t durationMs = 250;
    for (int32_t id : vibratorIds) {
        std::shared_ptr<IVibrator> vibrator;
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);
        EXPECT_OK(vibrator->on(durationMs, nullptr));
    }

    EXPECT_OK(session->close());

    auto timeout = std::chrono::milliseconds(durationMs) + VIBRATION_CALLBACK_TIMEOUT;
    EXPECT_EQ(sessionCallback->wait_for(timeout), std::future_status::ready);
}

TEST_P(VibratorManagerAidl, VibrationSessionEndMultipleTimes) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;
    if (vibratorIds.empty()) return;

    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>();
    VibrationSessionConfig sessionConfig;
    EXPECT_OK(manager->startSession(vibratorIds, sessionConfig, sessionCallback, &session));
    ASSERT_NE(session, nullptr);

    int32_t durationMs = 250;
    std::vector<std::shared_ptr<CompletionCallback>> vibrationCallbacks;
    for (int32_t id : vibratorIds) {
        std::shared_ptr<IVibrator> vibrator;
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);

        auto vibrationCallback = ndk::SharedRefBase::make<CompletionCallback>();
        vibrationCallbacks.push_back(vibrationCallback);
        EXPECT_OK(vibrator->on(durationMs, vibrationCallback));
    }

    // Session callback not triggered.
    EXPECT_EQ(sessionCallback->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::timeout);

    // End session, this might take a while
    EXPECT_OK(session->close());

    // End session again
    EXPECT_OK(session->close());

    // Both callbacks triggered within timeout.
    auto timeout = std::chrono::milliseconds(durationMs) + VIBRATION_CALLBACK_TIMEOUT;
    EXPECT_EQ(sessionCallback->wait_for(timeout), std::future_status::ready);
    for (auto& cb : vibrationCallbacks) {
        EXPECT_EQ(cb->wait_for(timeout), std::future_status::ready);
    }
}

TEST_P(VibratorManagerAidl, VibrationSessionDeletedAfterEnded) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;
    if (vibratorIds.empty()) return;

    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>();
    VibrationSessionConfig sessionConfig;
    EXPECT_OK(manager->startSession(vibratorIds, sessionConfig, sessionCallback, &session));
    ASSERT_NE(session, nullptr);

    int32_t durationMs = 250;
    std::vector<std::shared_ptr<CompletionCallback>> vibrationCallbacks;
    for (int32_t id : vibratorIds) {
        std::shared_ptr<IVibrator> vibrator;
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);

        auto vibrationCallback = ndk::SharedRefBase::make<CompletionCallback>();
        vibrationCallbacks.push_back(vibrationCallback);
        EXPECT_OK(vibrator->on(durationMs, vibrationCallback));
    }

    // Session callback not triggered.
    EXPECT_EQ(sessionCallback->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::timeout);

    // End session, this might take a while
    EXPECT_OK(session->close());

    session.reset();

    // Both callbacks triggered within timeout, even after session was deleted.
    auto timeout = std::chrono::milliseconds(durationMs) + VIBRATION_CALLBACK_TIMEOUT;
    EXPECT_EQ(sessionCallback->wait_for(timeout), std::future_status::ready);
    for (auto& cb : vibrationCallbacks) {
        EXPECT_EQ(cb->wait_for(timeout), std::future_status::ready);
    }
}

TEST_P(VibratorManagerAidl, VibrationSessionWrongVibratorIdsFail) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;

    auto maxIdIt = std::max_element(vibratorIds.begin(), vibratorIds.end());
    int32_t wrongId = maxIdIt == vibratorIds.end() ? 0 : *maxIdIt + 1;

    std::vector<int32_t> emptyIds;
    std::vector<int32_t> wrongIds{wrongId};
    VibrationSessionConfig sessionConfig;
    EXPECT_ILLEGAL_ARGUMENT(manager->startSession(emptyIds, sessionConfig, nullptr, &session));
    EXPECT_ILLEGAL_ARGUMENT(manager->startSession(wrongIds, sessionConfig, nullptr, &session));
    EXPECT_EQ(session, nullptr);
}

TEST_P(VibratorManagerAidl, VibrationSessionDuringPrepareSyncedFails) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;
    if (vibratorIds.empty()) return;

    EXPECT_OK(manager->prepareSynced(vibratorIds));

    VibrationSessionConfig sessionConfig;
    EXPECT_ILLEGAL_STATE(manager->startSession(vibratorIds, sessionConfig, nullptr, &session));
    EXPECT_EQ(session, nullptr);

    EXPECT_OK(manager->cancelSynced());
}

TEST_P(VibratorManagerAidl, VibrationSessionsUnsupported) {
    if (version < VIBRATION_SESSIONS_MIN_VERSION) {
        EXPECT_EQ(capabilities & IVibratorManager::CAP_START_SESSIONS, 0)
                << "Vibrator manager version " << version
                << " should not report start session capability";
    }
    if (capabilities & IVibratorManager::CAP_START_SESSIONS) return;

    VibrationSessionConfig sessionConfig;
    EXPECT_UNKNOWN_OR_UNSUPPORTED(
            manager->startSession(vibratorIds, sessionConfig, nullptr, &session));
    EXPECT_EQ(session, nullptr);
    EXPECT_UNKNOWN_OR_UNSUPPORTED(manager->clearSessions());
}

TEST_P(VibratorManagerAidl, HapticGeneratorSessionSuccess) {
    if (!(capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR)) return;
    if (vibratorIds.empty()) return;

    HapticGeneratorSession session;
    for (int32_t vibratorId : vibratorIds) {
        HapticGeneratorUtils::startHapticGeneratorSession(manager, vibratorId, nullptr, &session);
    }
}

TEST_P(VibratorManagerAidl, HapticGeneratorSessionSucceedsWithValidConfig) {
    if (!(capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR)) return;
    if (vibratorIds.empty()) return;

    HapticGeneratorConfig config1 = HapticGeneratorUtils::createConfig(
            48000 /* sampleRate */,
            AudioChannelLayout::make<AudioChannelLayout::Tag::layoutMask>(
                    AudioChannelLayout::CHANNEL_HAPTIC_A) /* channelMask */);

    HapticGeneratorSession hgSession1;
    EXPECT_OK(manager->startHapticGeneratorSession(vibratorIds, config1, nullptr, &hgSession1));
    ASSERT_FALSE(hgSession1.queues.empty());

    HapticGeneratorConfig config2 = HapticGeneratorUtils::createConfig(
            48000 /* sampleRate */,
            AudioChannelLayout::make<AudioChannelLayout::Tag::layoutMask>(
                    AudioChannelLayout::CHANNEL_HAPTIC_B) /* channelMask */);

    HapticGeneratorSession hgSession2;
    EXPECT_OK(manager->startHapticGeneratorSession(vibratorIds, config2, nullptr, &hgSession2));
    ASSERT_FALSE(hgSession2.queues.empty());
}

TEST_P(VibratorManagerAidl, HapticGeneratorSessionFailsWithInvalidSampleRate) {
    if (!(capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR)) return;
    if (vibratorIds.empty()) return;

    HapticGeneratorConfig config = HapticGeneratorUtils::createConfig(0 /* sampleRate */);

    HapticGeneratorSession session;
    EXPECT_ILLEGAL_ARGUMENT(
            manager->startHapticGeneratorSession(vibratorIds, config, nullptr, &session));
}

TEST_P(VibratorManagerAidl, HapticGeneratorSessionFailsWithNonPcmFormat) {
    if (!(capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR)) return;
    if (vibratorIds.empty()) return;

    // Create a config that is valid except for the format type.
    HapticGeneratorConfig config = HapticGeneratorUtils::createConfig(
            48000 /* sampleRate */,
            AudioChannelLayout::make<AudioChannelLayout::Tag::layoutMask>(
                    AudioChannelLayout::CHANNEL_HAPTIC_A) /* channelMask */,
            PcmType::INT_16_BIT /* pcmType */, AudioFormatType::NON_PCM /* formatType */);

    HapticGeneratorSession session;
    EXPECT_ILLEGAL_ARGUMENT(
            manager->startHapticGeneratorSession(vibratorIds, config, nullptr, &session));
}

TEST_P(VibratorManagerAidl, HapticGeneratorSessionFailsWithDefaultPcmType) {
    if (!(capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR)) return;
    if (vibratorIds.empty()) return;

    // Create a config that is valid except for the PCM type.
    HapticGeneratorConfig config = HapticGeneratorUtils::createConfig(
            48000 /* sampleRate */,
            AudioChannelLayout::make<AudioChannelLayout::Tag::layoutMask>(
                    AudioChannelLayout::CHANNEL_HAPTIC_A) /* channelMask */,
            PcmType::DEFAULT /* pcmType */);

    HapticGeneratorSession session;
    EXPECT_ILLEGAL_ARGUMENT(
            manager->startHapticGeneratorSession(vibratorIds, config, nullptr, &session));
}

TEST_P(VibratorManagerAidl, HapticGeneratorSessionFailsWithNonHapticChannelMask) {
    if (!(capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR)) return;
    if (vibratorIds.empty()) return;

    // Create a config that is valid except for the channel mask.
    HapticGeneratorConfig config = HapticGeneratorUtils::createConfig(
            48000 /* sampleRate */, AudioChannelLayout::make<AudioChannelLayout::Tag::layoutMask>(
                                            AudioChannelLayout::LAYOUT_STEREO) /* channelMask */);

    HapticGeneratorSession session;
    EXPECT_ILLEGAL_ARGUMENT(
            manager->startHapticGeneratorSession(vibratorIds, config, nullptr, &session));
}

TEST_P(VibratorManagerAidl, HapticGeneratorSessionMultipleVibratorsSuccess) {
    if (!(capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR)) return;
    if (vibratorIds.size() <= 1) return;

    HapticGeneratorConfig config;
    HapticGeneratorSession session;
    EXPECT_OK(manager->startHapticGeneratorSession(vibratorIds, config, nullptr, &session));
    ASSERT_EQ(session.queues.size(), vibratorIds.size());

    for (size_t i = 0; i < vibratorIds.size(); ++i) {
        const auto& queues = session.queues[i];
        HapticGeneratorUtils::assertQueuesAreValidForVibratorId(queues, vibratorIds[i]);
    }
}

TEST_P(VibratorManagerAidl, HapticGeneratorSessionFailsIfUnsupported) {
    if (capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR) return;
    if (vibratorIds.empty()) return;

    HapticGeneratorConfig config;
    HapticGeneratorSession session;

    EXPECT_UNKNOWN_OR_UNSUPPORTED(
            manager->startHapticGeneratorSession(vibratorIds, config, nullptr, &session));
}

TEST_P(VibratorManagerAidl, HapticGeneratorSessionFailsWithInvalidVibratorId) {
    if (!(capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR)) return;

    int32_t invalidId = *max_element(vibratorIds.begin(), vibratorIds.end()) + 1;
    const std::vector<int32_t> invalidVibratorIds = {invalidId};

    HapticGeneratorConfig config;
    HapticGeneratorSession session;
    EXPECT_ILLEGAL_ARGUMENT(
            manager->startHapticGeneratorSession(invalidVibratorIds, config, nullptr, &session));
}

TEST_P(VibratorManagerAidl, HapticGeneratorSessionFailsWithEmptyVibratorId) {
    if (!(capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR)) return;

    HapticGeneratorConfig config;
    HapticGeneratorSession session;
    EXPECT_ILLEGAL_ARGUMENT(manager->startHapticGeneratorSession({}, config, nullptr, &session));
}

TEST_P(VibratorManagerAidl, HapticGeneratorStartMultipleSessionsOnSameVibrator) {
    if (!(capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR)) return;
    if (vibratorIds.empty()) return;

    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>();
    HapticGeneratorSession session1;
    HapticGeneratorUtils::HapticGeneratorUtils::startHapticGeneratorSession(
            manager, vibratorIds[0], sessionCallback, &session1);

    HapticGeneratorSession session2;
    HapticGeneratorUtils::startHapticGeneratorSession(manager, vibratorIds[0], nullptr, &session2);

    // First session was not cancelled
    EXPECT_EQ(sessionCallback->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::timeout);

    ASSERT_NE(session1.queues[0].command, session2.queues[0].command);
}

TEST_P(VibratorManagerAidl, HapticGeneratorStartAndCloseSession) {
    if (!(capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR)) return;
    if (vibratorIds.empty()) return;

    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>();
    HapticGeneratorSession hgSession;
    HapticGeneratorUtils::startHapticGeneratorSession(manager, vibratorIds[0], sessionCallback,
                                                      &hgSession);

    auto commandQueue = std::make_unique<CommandQueue>(hgSession.queues[0].command);
    auto replyQueue = std::make_unique<ReplyQueue>(hgSession.queues[0].reply);

    HapticGeneratorUtils::sendCloseCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);
    // Session callback triggered
    EXPECT_EQ(sessionCallback->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::ready);

    // Send another command and verify the session is unresponsive
    HapticGeneratorCommand burstCmd;
    burstCmd.set<HapticGeneratorCommand::Tag::burstBytes>(128);
    ASSERT_TRUE(commandQueue->writeBlocking(&burstCmd, 1, FMQ_TIMEOUT_NANOS));

    // Since the HAL thread is gone, no reply will ever be sent. This blocking read
    // call MUST time out and return false.
    HapticGeneratorReply reply;
    EXPECT_FALSE(replyQueue->readBlocking(&reply, 1, FMQ_TIMEOUT_NANOS));
}

TEST_P(VibratorManagerAidl, HapticGeneratorEnqueueAllVibrationEffectTypes) {
    if (!(capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR)) return;
    if (vibratorIds.empty()) return;

    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>();
    HapticGeneratorSession hgSession;
    HapticGeneratorUtils::startHapticGeneratorSession(manager, vibratorIds[0], sessionCallback,
                                                      &hgSession);
    auto commandQueue = std::make_unique<CommandQueue>(hgSession.queues[0].command);
    auto effectQueue = std::make_unique<EffectQueue>(hgSession.queues[0].effect);
    auto replyQueue = std::make_unique<ReplyQueue>(hgSession.queues[0].reply);
    auto pcmQueue = std::make_unique<PcmQueue>(hgSession.queues[0].pcm);

    HapticGeneratorUtils::sendStartCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);
    EXPECT_EQ(pcmQueue->availableToRead(), 0);

    VibrationEffect predefinedEffect;
    predefinedEffect.set<VibrationEffect::Tag::predefined>(PredefinedEffect{
            .effect = kEffects[0],
    });
    EXPECT_TRUE(effectQueue->writeBlocking(&predefinedEffect, 1, FMQ_TIMEOUT_NANOS));

    VibrationEffect oneShotPrimitiveEffect;
    oneShotPrimitiveEffect.set<VibrationEffect::Tag::oneShotPrimitive>(OneShotPrimitive{
            .amplitude = 0.5f,
            .timeMillis = 100,
    });
    EXPECT_TRUE(effectQueue->writeBlocking(&oneShotPrimitiveEffect, 1, FMQ_TIMEOUT_NANOS));

    VibrationEffect compositeEffect;
    compositeEffect.set<VibrationEffect::Tag::composite>(CompositeEffect{
            .delayMs = 10,
            .primitive = CompositePrimitive::NOOP,
            .scale = 0.0f,
    });
    EXPECT_TRUE(effectQueue->writeBlocking(&compositeEffect, 1, FMQ_TIMEOUT_NANOS));

    VibrationEffect pwleEffect;
    PwleV2Primitive pwle;
    pwleEffect.set<VibrationEffect::Tag::pwleV2Primitive>(pwle);
    EXPECT_TRUE(effectQueue->writeBlocking(&pwleEffect, 1, FMQ_TIMEOUT_NANOS));

    HapticGeneratorUtils::sendCompleteCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);

    size_t totalBytesReceived =
            HapticGeneratorUtils::drainPcmQueue(commandQueue, replyQueue, pcmQueue);

    // Verify at least some data was received
    EXPECT_GT(totalBytesReceived, 0);

    HapticGeneratorUtils::sendCloseCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);
    // Session callback triggered
    EXPECT_EQ(sessionCallback->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::ready);
}

TEST_P(VibratorManagerAidl, HapticGeneratorCommandFailsWithoutStartEffect) {
    if (!(capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR)) return;
    if (vibratorIds.empty()) return;

    HapticGeneratorSession hgSession;
    HapticGeneratorUtils::startHapticGeneratorSession(manager, vibratorIds[0], nullptr, &hgSession);
    auto commandQueue = std::make_unique<CommandQueue>(hgSession.queues[0].command);
    auto replyQueue = std::make_unique<ReplyQueue>(hgSession.queues[0].reply);

    // Test burstBytes command
    HapticGeneratorUtils::sendBurstCommandExpectStatusReply(commandQueue, replyQueue, 128,
                                                            STATUS_INVALID_OPERATION);

    // Test completeEffect command
    HapticGeneratorUtils::sendCompleteCommandExpectStatusReply(commandQueue, replyQueue,
                                                               STATUS_INVALID_OPERATION);

    // Test cancelEffect command
    HapticGeneratorUtils::sendCancelCommandExpectStatusReply(commandQueue, replyQueue,
                                                             STATUS_INVALID_OPERATION);

    HapticGeneratorUtils::sendCloseCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);
}

TEST_P(VibratorManagerAidl, HapticGeneratorBurstWithoutEffectShouldNotFail) {
    if (!(capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR)) return;
    if (vibratorIds.empty()) return;

    HapticGeneratorSession hgSession;
    HapticGeneratorUtils::startHapticGeneratorSession(manager, vibratorIds[0], nullptr, &hgSession);
    auto commandQueue = std::make_unique<CommandQueue>(hgSession.queues[0].command);
    auto replyQueue = std::make_unique<ReplyQueue>(hgSession.queues[0].reply);
    auto pcmQueue = std::make_unique<PcmQueue>(hgSession.queues[0].pcm);

    HapticGeneratorUtils::sendStartCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);
    EXPECT_EQ(pcmQueue->availableToRead(), 0);

    // Calling burstBytes command multiple times without an effect in queue should not fail.
    // The reply status should be STATUS_NOT_ENOUGH_DATA.
    const size_t burstSize = 128;
    for (size_t i = 0; i < 3; ++i) {
        HapticGeneratorUtils::sendBurstCommandExpectStatusReply(commandQueue, replyQueue, burstSize,
                                                                STATUS_NOT_ENOUGH_DATA);
    }

    HapticGeneratorUtils::sendCloseCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);
}

TEST_P(VibratorManagerAidl, HapticGeneratorStartEffectAndBurstFullEffect) {
    if (!(capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR)) return;
    if (vibratorIds.empty()) return;

    HapticGeneratorSession hgSession;
    HapticGeneratorUtils::startHapticGeneratorSession(manager, vibratorIds[0], nullptr, &hgSession);
    auto commandQueue = std::make_unique<CommandQueue>(hgSession.queues[0].command);
    auto effectQueue = std::make_unique<EffectQueue>(hgSession.queues[0].effect);
    auto replyQueue = std::make_unique<ReplyQueue>(hgSession.queues[0].reply);
    auto pcmQueue = std::make_unique<PcmQueue>(hgSession.queues[0].pcm);

    HapticGeneratorUtils::sendStartCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);
    EXPECT_EQ(pcmQueue->availableToRead(), 0);

    VibrationEffect effect;
    effect.set<VibrationEffect::Tag::predefined>(PredefinedEffect{});
    ASSERT_TRUE(effectQueue->writeBlocking(&effect, 1, FMQ_TIMEOUT_NANOS));

    size_t totalBytesReceived = 0;
    const size_t burstSize = 128;
    std::vector<int8_t> pcmData(burstSize);
    size_t bytesReceived = 0;
    int status = STATUS_OK;

    // Write first effect
    ASSERT_TRUE(effectQueue->writeBlocking(&effect, 1, FMQ_TIMEOUT_NANOS));

    // Send burst command and read PCM if data was returned
    HapticGeneratorUtils::executeBurstCommand(commandQueue, replyQueue, burstSize, &bytesReceived,
                                              &status);
    ASSERT_TRUE(status == STATUS_OK || status == STATUS_NOT_ENOUGH_DATA);
    if (status == STATUS_OK && bytesReceived > 0) {
        EXPECT_LE(bytesReceived, burstSize);
        HapticGeneratorUtils::readPcmData(pcmQueue, bytesReceived, &pcmData);
        totalBytesReceived += bytesReceived;
    }

    // Write second effect
    ASSERT_TRUE(effectQueue->writeBlocking(&effect, 1, FMQ_TIMEOUT_NANOS));

    // Send effect complete
    HapticGeneratorUtils::sendCompleteCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);

    // Send burst commands and read PCM until full effect received
    totalBytesReceived += HapticGeneratorUtils::drainPcmQueue(commandQueue, replyQueue, pcmQueue);

    // Verify at least some data was received
    EXPECT_GT(totalBytesReceived, 0);

    HapticGeneratorUtils::sendCloseCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);
}

TEST_P(VibratorManagerAidl, HapticGeneratorStreamLongEffect) {
    if (!(capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR)) {
        return;
    }

    HapticGeneratorSession hgSession;
    HapticGeneratorUtils::startHapticGeneratorSession(manager, vibratorIds[0], nullptr, &hgSession);
    auto commandQueue = std::make_unique<CommandQueue>(hgSession.queues[0].command);
    auto effectQueue = std::make_unique<EffectQueue>(hgSession.queues[0].effect);
    auto replyQueue = std::make_unique<ReplyQueue>(hgSession.queues[0].reply);
    auto pcmQueue = std::make_unique<PcmQueue>(hgSession.queues[0].pcm);

    HapticGeneratorUtils::sendStartCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);

    // Fill the effect queue completely with the first half of the effect
    VibrationEffect effect;
    effect.set<VibrationEffect::Tag::predefined>(PredefinedEffect{});
    const size_t effectQueueCapacity = effectQueue->getQuantumCount();
    for (size_t i = 0; i < effectQueueCapacity; ++i) {
        ASSERT_TRUE(effectQueue->writeBlocking(&effect, 1, FMQ_TIMEOUT_NANOS));
    }

    size_t bytesReceived = 0;
    int status = 0;
    const size_t burstSize = 1024;
    std::vector<int8_t> pcmData(burstSize);
    size_t totalBytesReceived = 0;

    HapticGeneratorUtils::executeBurstCommand(commandQueue, replyQueue, burstSize, &bytesReceived,
                                              &status);
    if (status == STATUS_OK) {
        EXPECT_LE(bytesReceived, burstSize);
        HapticGeneratorUtils::readPcmData(pcmQueue, bytesReceived, &pcmData);
        ASSERT_GE(bytesReceived, 0);
        totalBytesReceived += bytesReceived;
    }

    // After doing a burst command, the effect queue should be empty.
    ASSERT_EQ(effectQueue->availableToRead(), 0);

    // Now we send the second half of the effect
    for (size_t i = 0; i < effectQueueCapacity; ++i) {
        ASSERT_TRUE(effectQueue->writeBlocking(&effect, 1, FMQ_TIMEOUT_NANOS));
    }

    // Notify the HAL that the entire effect has been sent
    HapticGeneratorUtils::sendCompleteCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);

    totalBytesReceived += HapticGeneratorUtils::drainPcmQueue(commandQueue, replyQueue, pcmQueue);

    EXPECT_GT(totalBytesReceived, 0);

    HapticGeneratorUtils::sendCloseCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);
}

TEST_P(VibratorManagerAidl, HapticGeneratorRestartEffectMidConversion) {
    if (!(capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR)) return;
    if (vibratorIds.empty()) return;

    HapticGeneratorSession hgSession;
    HapticGeneratorUtils::startHapticGeneratorSession(manager, vibratorIds[0], nullptr, &hgSession);
    auto commandQueue = std::make_unique<CommandQueue>(hgSession.queues[0].command);
    auto effectQueue = std::make_unique<EffectQueue>(hgSession.queues[0].effect);
    auto replyQueue = std::make_unique<ReplyQueue>(hgSession.queues[0].reply);
    auto pcmQueue = std::make_unique<PcmQueue>(hgSession.queues[0].pcm);

    HapticGeneratorUtils::sendStartCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);
    EXPECT_EQ(pcmQueue->availableToRead(), 0);

    VibrationEffect effect;
    effect.set<VibrationEffect::Tag::predefined>(PredefinedEffect{});
    ASSERT_TRUE(effectQueue->writeBlocking(&effect, 1, FMQ_TIMEOUT_NANOS));

    size_t bytesReceived = 0;
    int status = 0;
    HapticGeneratorUtils::executeBurstCommand(commandQueue, replyQueue, 1024, &bytesReceived,
                                              &status);

    // Restart with a new effect
    HapticGeneratorUtils::sendStartCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);
    EXPECT_EQ(pcmQueue->availableToRead(), 0);

    ASSERT_TRUE(effectQueue->writeBlocking(&effect, 1, FMQ_TIMEOUT_NANOS));

    HapticGeneratorUtils::sendCompleteCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);

    size_t totalBytesReceived =
            HapticGeneratorUtils::drainPcmQueue(commandQueue, replyQueue, pcmQueue);

    EXPECT_GT(totalBytesReceived, 0);

    HapticGeneratorUtils::sendCloseCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);
}

TEST_P(VibratorManagerAidl, HapticGeneratorCancelEffectMidConversion) {
    if (!(capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR)) return;
    if (vibratorIds.empty()) return;

    HapticGeneratorSession hgSession;
    HapticGeneratorUtils::startHapticGeneratorSession(manager, vibratorIds[0], nullptr, &hgSession);
    auto commandQueue = std::make_unique<CommandQueue>(hgSession.queues[0].command);
    auto effectQueue = std::make_unique<EffectQueue>(hgSession.queues[0].effect);
    auto replyQueue = std::make_unique<ReplyQueue>(hgSession.queues[0].reply);
    auto pcmQueue = std::make_unique<PcmQueue>(hgSession.queues[0].pcm);

    HapticGeneratorUtils::sendStartCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);

    EXPECT_EQ(pcmQueue->availableToRead(), 0);

    VibrationEffect effect;
    effect.set<VibrationEffect::Tag::predefined>(PredefinedEffect{});
    ASSERT_TRUE(effectQueue->writeBlocking(&effect, 1, FMQ_TIMEOUT_NANOS));

    size_t bytesReceived = 0;
    int status = 0;
    HapticGeneratorUtils::executeBurstCommand(commandQueue, replyQueue, 1024, &bytesReceived,
                                              &status);

    HapticGeneratorUtils::sendCancelCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);
    EXPECT_EQ(pcmQueue->availableToRead(), 0);

    HapticGeneratorUtils::sendBurstCommandExpectStatusReply(commandQueue, replyQueue, 1024,
                                                            STATUS_INVALID_OPERATION);

    HapticGeneratorUtils::sendCloseCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);
}

TEST_P(VibratorManagerAidl, HapticGeneratorSessionCleared) {
    if (!(capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR)) return;
    if (vibratorIds.empty()) return;

    auto callback = ndk::SharedRefBase::make<CompletionCallback>();
    HapticGeneratorSession hgSession;
    HapticGeneratorUtils::startHapticGeneratorSession(manager, vibratorIds[0], callback,
                                                      &hgSession);

    auto commandQueue = std::make_unique<CommandQueue>(hgSession.queues[0].command);
    auto effectQueue = std::make_unique<EffectQueue>(hgSession.queues[0].effect);
    auto replyQueue = std::make_unique<ReplyQueue>(hgSession.queues[0].reply);
    auto pcmQueue = std::make_unique<PcmQueue>(hgSession.queues[0].pcm);

    VibrationEffect effect;
    effect.set<VibrationEffect::Tag::predefined>(PredefinedEffect{});
    ASSERT_TRUE(effectQueue->writeBlocking(&effect, 1, FMQ_TIMEOUT_NANOS));

    HapticGeneratorUtils::sendStartCommandExpectStatusReply(commandQueue, replyQueue, STATUS_OK);
    EXPECT_EQ(pcmQueue->availableToRead(), 0);

    EXPECT_OK(manager->clearSessions());
    // Session callback triggered.
    EXPECT_EQ(callback->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::ready);
}

TEST_P(VibratorManagerAidl, HapticGeneratorAndVibrationSessionsCleared) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS) ||
        !(capabilities & IVibratorManager::CAP_HAPTIC_GENERATOR)) {
        return;
    }
    if (vibratorIds.empty()) return;

    // Start a vibration session.
    auto vibrationSessionCallback = ndk::SharedRefBase::make<CompletionCallback>();
    VibrationSessionConfig sessionConfig;
    std::shared_ptr<IVibrationSession> vibrationSession;
    EXPECT_OK(manager->startSession(vibratorIds, sessionConfig, vibrationSessionCallback,
                                    &vibrationSession));
    ASSERT_NE(vibrationSession, nullptr);

    // Start a haptic generator session
    auto hgSessionCallback = ndk::SharedRefBase::make<CompletionCallback>();
    HapticGeneratorSession hgSession;
    HapticGeneratorUtils::startHapticGeneratorSession(manager, vibratorIds[0], hgSessionCallback,
                                                      &hgSession);

    // Clear all sessions
    EXPECT_OK(manager->clearSessions());

    // Both session callbacks were triggered
    EXPECT_EQ(vibrationSessionCallback->wait_for(VIBRATION_CALLBACK_TIMEOUT),
              std::future_status::ready);
    EXPECT_EQ(hgSessionCallback->wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::ready);
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

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VibratorManagerAidl);
INSTANTIATE_TEST_SUITE_P(VibratorManager, VibratorManagerAidl,
                         testing::ValuesIn(FindVibratorManagerNames()),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(2);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
