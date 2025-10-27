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

#ifndef VIBRATOR_HAL_HAPTIC_GENERATOR_UTILS_H
#define VIBRATOR_HAL_HAPTIC_GENERATOR_UTILS_H

#include <aidl/Gtest.h>
#include <aidl/android/hardware/vibrator/HapticGeneratorCommand.h>
#include <aidl/android/hardware/vibrator/HapticGeneratorReply.h>
#include <aidl/android/hardware/vibrator/HapticGeneratorSession.h>
#include <aidl/android/hardware/vibrator/IVibratorCallback.h>
#include <aidl/android/hardware/vibrator/IVibratorManager.h>
#include <aidl/android/hardware/vibrator/VibrationEffectContent.h>
#include <aidl/android/media/audio/common/AudioChannelLayout.h>
#include <aidl/android/media/audio/common/AudioConfigBase.h>
#include <aidl/android/media/audio/common/AudioFormatDescription.h>
#include <aidl/android/media/audio/common/AudioFormatType.h>
#include <aidl/android/media/audio/common/PcmType.h>
#include <fmq/AidlMessageQueue.h>
#include <gtest/gtest.h>
#include "test_utils.h"

#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <memory>
#include <vector>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {
namespace testing {
namespace hapticgenerator {

namespace fmq = aidl::android::hardware::common::fmq;

using ::aidl::android::media::audio::common::AudioChannelLayout;
using ::aidl::android::media::audio::common::AudioFormatDescription;
using ::aidl::android::media::audio::common::AudioFormatType;
using ::aidl::android::media::audio::common::PcmType;

// FMQ aliases
using CommandQueue =
        ::android::AidlMessageQueue<HapticGeneratorCommand, fmq::SynchronizedReadWrite>;
using EffectQueue = ::android::AidlMessageQueue<VibrationEffectContent, fmq::SynchronizedReadWrite>;
using ReplyQueue = ::android::AidlMessageQueue<HapticGeneratorReply, fmq::SynchronizedReadWrite>;
using PcmQueue = ::android::AidlMessageQueue<int8_t, fmq::SynchronizedReadWrite>;

class HapticGeneratorUtils {
  public:
    static void startHapticGeneratorSession(const std::shared_ptr<IVibratorManager>& manager,
                                            const int32_t vibratorId,
                                            const std::shared_ptr<IVibratorCallback>& callback,
                                            HapticGeneratorSession* hgSession) {
        HapticGeneratorConfig config = createConfig();

        EXPECT_OK(manager->startHapticGeneratorSession({vibratorId}, config, callback, hgSession));
        ASSERT_FALSE(hgSession->queues.empty());
        const auto& queues = hgSession->queues[0];

        assertQueuesAreValidForVibratorId(queues, vibratorId);
    }

    static HapticGeneratorConfig createConfig(
            int32_t sampleRate = 48000,
            const AudioChannelLayout& channelMask =
                    AudioChannelLayout::make<AudioChannelLayout::Tag::layoutMask>(
                            AudioChannelLayout::LAYOUT_HAPTIC_AB),
            PcmType pcmType = PcmType::INT_16_BIT,
            AudioFormatType formatType = AudioFormatType::PCM) {
        HapticGeneratorConfig config;
        config.audioFormat = {
                .sampleRate = sampleRate,
                .channelMask = channelMask,
                .format =
                        {
                                .type = formatType,
                                .pcm = pcmType,
                                .encoding = "",
                        },
        };
        return config;
    }

    static void assertQueuesAreValidForVibratorId(const HapticGeneratorQueues& queues,
                                                  int32_t vibratorId) {
        EXPECT_EQ(queues.vibratorId, vibratorId);
        ASSERT_TRUE(CommandQueue(queues.command).isValid());
        ASSERT_TRUE(EffectQueue(queues.effect).isValid());
        ASSERT_TRUE(ReplyQueue(queues.reply).isValid());
        ASSERT_TRUE(PcmQueue(queues.pcm).isValid());
    }

    static void sendStartCommandExpectStatusReply(const std::unique_ptr<CommandQueue>& commandQueue,
                                                  const std::unique_ptr<ReplyQueue>& replyQueue,
                                                  int expectedStatus) {
        HapticGeneratorCommand startCmd;
        startCmd.set<HapticGeneratorCommand::Tag::effect>(HapticGeneratorCommand::Effect::START);
        ASSERT_TRUE(commandQueue->writeBlocking(&startCmd, 1, FMQ_TIMEOUT_NANOS));
        receiveReply(replyQueue, expectedStatus);
    }

    static void executeBurstCommand(const std::unique_ptr<CommandQueue>& commandQueue,
                                    const std::unique_ptr<ReplyQueue>& replyQueue, size_t burstSize,
                                    size_t* bytesReceived, int* status) {
        sendBurstCommand(commandQueue, burstSize);
        receiveReply(replyQueue, bytesReceived, status);
    }

    static void sendBurstCommandExpectStatusReply(const std::unique_ptr<CommandQueue>& commandQueue,
                                                  const std::unique_ptr<ReplyQueue>& replyQueue,
                                                  size_t burstSize, int expectedStatus) {
        sendBurstCommand(commandQueue, burstSize);
        receiveReply(replyQueue, expectedStatus);
    }

    static void sendCompleteCommandExpectStatusReply(
            const std::unique_ptr<CommandQueue>& commandQueue,
            const std::unique_ptr<ReplyQueue>& replyQueue, int expectedStatus) {
        HapticGeneratorCommand completeCmd;
        completeCmd.set<HapticGeneratorCommand::Tag::effect>(
                HapticGeneratorCommand::Effect::COMPLETE);
        ASSERT_TRUE(commandQueue->writeBlocking(&completeCmd, 1, FMQ_TIMEOUT_NANOS));
        receiveReply(replyQueue, expectedStatus);
    }

    static void sendCancelCommandExpectStatusReply(
            const std::unique_ptr<CommandQueue>& commandQueue,
            const std::unique_ptr<ReplyQueue>& replyQueue, int expectedStatus) {
        HapticGeneratorCommand cancelCmd;
        cancelCmd.set<HapticGeneratorCommand::Tag::effect>(HapticGeneratorCommand::Effect::CANCEL);
        ASSERT_TRUE(commandQueue->writeBlocking(&cancelCmd, 1, FMQ_TIMEOUT_NANOS));
        receiveReply(replyQueue, expectedStatus);
    }

    static void sendCloseCommandExpectStatusReply(const std::unique_ptr<CommandQueue>& commandQueue,
                                                  const std::unique_ptr<ReplyQueue>& replyQueue,
                                                  int expectedStatus) {
        HapticGeneratorCommand command;
        command.set<HapticGeneratorCommand::Tag::session>(HapticGeneratorCommand::Session::CLOSE);
        ASSERT_TRUE(commandQueue->writeBlocking(&command, 1, FMQ_TIMEOUT_NANOS));
        receiveReply(replyQueue, expectedStatus);
    }

    static void readPcmData(const std::unique_ptr<PcmQueue>& pcmQueue, size_t bytesToRead,
                            std::vector<int8_t>* pcmData) {
        ASSERT_GE(pcmData->size(), bytesToRead);
        if (bytesToRead > 0) {
            ASSERT_TRUE(pcmQueue->read(pcmData->data(), bytesToRead));
        }
    }

    static size_t drainPcmQueue(const std::unique_ptr<CommandQueue>& commandQueue,
                                const std::unique_ptr<ReplyQueue>& replyQueue,
                                const std::unique_ptr<PcmQueue>& pcmQueue) {
        size_t totalBytesReceived = 0;
        const size_t burstSize = 1024;
        std::vector<int8_t> pcmData(burstSize);

        while (true) {
            size_t bytesReceived = 0;
            int status = STATUS_OK;
            executeBurstCommand(commandQueue, replyQueue, burstSize, &bytesReceived, &status);

            // The HAL is expected to be either providing data (OK) or waiting for more
            // (NOT_ENOUGH_DATA). Any other status indicates a critical failure.
            EXPECT_TRUE(status == STATUS_OK || status == STATUS_NOT_ENOUGH_DATA)
                    << "Expected STATUS_OK or STATUS_NOT_ENOUGH_DATA, but got " << status;

            if (status == STATUS_OK) {
                if (bytesReceived == 0) {
                    break;  // Effect conversion finished.
                }
                EXPECT_LE(bytesReceived, burstSize);
                readPcmData(pcmQueue, bytesReceived, &pcmData);
                totalBytesReceived += bytesReceived;
            }
        }

        return totalBytesReceived;
    }

    static void clearPcmQueue(const std::unique_ptr<PcmQueue>& pcmQueue) {
        std::vector<int8_t> pcmBuffer(pcmQueue->getQuantumSize());
        while (pcmQueue->read(pcmBuffer.data(), pcmBuffer.size())) {
            // Discard data
        }
    }

  private:
    static void sendBurstCommand(const std::unique_ptr<CommandQueue>& commandQueue,
                                 size_t burstSize) {
        HapticGeneratorCommand burstCmd;
        burstCmd.set<HapticGeneratorCommand::Tag::burstBytes>(burstSize);
        ASSERT_TRUE(commandQueue->writeBlocking(&burstCmd, 1, FMQ_TIMEOUT_NANOS));
    }

    static void receiveReply(const std::unique_ptr<ReplyQueue>& replyQueue, size_t* bytesReady,
                             int* status) {
        HapticGeneratorReply reply;
        ASSERT_TRUE(replyQueue->readBlocking(&reply, 1, FMQ_TIMEOUT_NANOS));
        *bytesReady = reply.burstBytesReady;
        *status = reply.status;
    }

    static void receiveReply(const std::unique_ptr<ReplyQueue>& replyQueue, int expectedStatus) {
        size_t bytesReceived = 0;
        int status = 0;
        receiveReply(replyQueue, &bytesReceived, &status);
        EXPECT_EQ(status, expectedStatus);
    }
};

}  // namespace hapticgenerator
}  // namespace testing
}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
#endif  // VIBRATOR_HAL_HAPTIC_GENERATOR_UTILS_H
