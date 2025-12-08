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

#define LOG_TAG "ModuleBluetoothTest"

#include <android-base/logging.h>
#include <gtest/gtest.h>

#include <atomic>
#include <set>

#include <ModuleConfig.h>
#include <TestUtils.h>
#include <core-impl/Configuration.h>
#include <core-impl/ModuleBluetoothBase.h>

using aidl::android::hardware::audio::core::AudioPatch;
using aidl::android::hardware::audio::core::IStreamCommon;
using aidl::android::hardware::audio::core::IStreamOut;
using aidl::android::hardware::audio::core::Module;
using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioDeviceAddress;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::AudioInputFlags;
using aidl::android::media::audio::common::AudioIoFlags;
using aidl::android::media::audio::common::AudioLatencyMode;
using aidl::android::media::audio::common::AudioOutputFlags;
using aidl::android::media::audio::common::AudioPort;
using aidl::android::media::audio::common::AudioPortConfig;
using aidl::android::media::audio::common::AudioPortDeviceExt;
using aidl::android::media::audio::common::AudioPortExt;
using aidl::android::media::audio::common::AudioPortMixExt;
using aidl::android::media::audio::common::Int;
using aidl::android::media::audio::common::PcmType;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;

using OpenInputStreamArguments =
        aidl::android::hardware::audio::core::IModule::OpenInputStreamArguments;
using OpenInputStreamReturn = aidl::android::hardware::audio::core::IModule::OpenInputStreamReturn;
using OpenOutputStreamArguments =
        aidl::android::hardware::audio::core::IModule::OpenOutputStreamArguments;
using OpenOutputStreamReturn =
        aidl::android::hardware::audio::core::IModule::OpenOutputStreamReturn;

namespace android::bluetooth::audio::aidl {

class BluetoothAudioPortMock : public BluetoothAudioPort {
  public:
    BluetoothAudioPortMock() = default;
    virtual ~BluetoothAudioPortMock() { unregisterPort(); }

    bool registerPort(const ::aidl::android::media::audio::common::AudioDeviceDescription&
                              description) override {
        if (mInstances.count(description)) {
            LOG(DEBUG) << __func__ << ": Proxy for " << description << " already exists";
            return false;
        }
        mDescription = description;
        LOG(DEBUG) << __func__ << ": " << description;
        return true;
    }

    void unregisterPort() override {
        LOG(DEBUG) << __func__;
        mInstances.erase(mDescription);
    }

    bool loadAudioConfig(
            ::aidl::android::hardware::bluetooth::audio::PcmConfiguration& audio_cfg) override {
        audio_cfg.sampleRateHz = 44100;
        audio_cfg.channelMode = ::aidl::android::hardware::bluetooth::audio::ChannelMode::STEREO;
        audio_cfg.bitsPerSample = 16;
        audio_cfg.dataIntervalUs = 10000;
        return true;
    }

    bool standby() override { return true; }
    bool start() override { return true; }
    bool suspend() override { return true; }
    void stop() override {}

    BluetoothStreamState getState() const override { return mState; }

    bool setState(BluetoothStreamState state) override {
        mState = state;
        return true;
    }

    bool getPresentationPosition(::aidl::android::hardware::bluetooth::audio::PresentationPosition&
                                         presentation_position) const override {
        presentation_position.remoteDeviceAudioDelayNanos = 0;
        presentation_position.transmittedOctets = 0;
        return true;
    }

    bool updateSourceMetadata(
            const ::aidl::android::hardware::audio::common::SourceMetadata& /*sourceMetadata*/)
            const override {
        return true;
    }

    bool updateSinkMetadata(
            const ::aidl::android::hardware::audio::common::SinkMetadata& /*sinkMetadata*/)
            const override {
        return true;
    }

    bool isA2dp() const override { return false; }
    bool isLeAudio() const override { return false; }

    bool getPreferredDataIntervalUs(size_t& interval_us) const override {
        interval_us = 10000;  // Default 10ms
        return true;
    }

    bool getRecommendedLatencyModes(
            std::vector<::aidl::android::hardware::bluetooth::audio::LatencyMode>*) {
        return true;
    }

    std::string getSessionNameForDebug() const override { return "BluetoothAudioPortMock"; }

    void setCallbacks(const std::shared_ptr<BluetoothAudioPortCallbacks>& /*callbacks*/) override {}

  private:
    static std::set<AudioDeviceDescription> mInstances;

    AudioDeviceDescription mDescription;
    std::atomic<BluetoothStreamState> mState = BluetoothStreamState::STANDBY;
};

std::set<AudioDeviceDescription> BluetoothAudioPortMock::mInstances;

}  // namespace android::bluetooth::audio::aidl

using android::bluetooth::audio::aidl::BluetoothAudioPortMock;

namespace aidl::android::hardware::audio::core {

class ModuleBluetoothMock : public ModuleBluetoothBase {
  public:
    ModuleBluetoothMock(std::unique_ptr<Configuration>&& config)
        : ModuleBluetoothBase(std::move(config)) {}

    using ModuleBluetoothBase::connectExternalDevice;
    using ModuleBluetoothBase::openInputStream;
    using ModuleBluetoothBase::openOutputStream;
    using ModuleBluetoothBase::resetAudioPatch;
    using ModuleBluetoothBase::setAudioPatch;
    using ModuleBluetoothBase::setAudioPortConfig;

  protected:
    std::shared_ptr<::android::bluetooth::audio::aidl::BluetoothAudioPort> createProxyInstance(
            bool /*isInput*/) override {
        return std::make_shared<BluetoothAudioPortMock>();
    }
};

}  // namespace aidl::android::hardware::audio::core

using aidl::android::hardware::audio::core::ModuleBluetoothMock;

class ModuleBluetoothTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto config = ::aidl::android::hardware::audio::core::internal::getConfiguration(
                Module::Type::BLUETOOTH);
        module = ndk::SharedRefBase::make<ModuleBluetoothMock>(std::move(config));
        moduleConfig = std::make_unique<ModuleConfig>(module.get());
        ASSERT_EQ(EX_NONE, moduleConfig->getStatus().getExceptionCode())
                << "ModuleConfig init error: " << moduleConfig->getError();
    }

    void connectExternalDevice(const AudioPort& templatePort, AudioPort* connectedPort) {
        // Create a unique address to ensure a new connection
        AudioPort portWithAddress = templatePort;
        portWithAddress.ext.template get<AudioPortExt::Tag::device>().device.address =
                AudioDeviceAddress::make<AudioDeviceAddress::mac>(
                        std::vector<uint8_t>{1, 2, 3, 4, 5, 6});

        ASSERT_IS_OK(module->connectExternalDevice(portWithAddress, connectedPort));
        ASSERT_IS_OK(moduleConfig->onExternalDeviceConnected(module.get(), *connectedPort));
    }

    template <typename Stream>
    void closeStream(std::shared_ptr<Stream>& stream) {
        std::shared_ptr<IStreamCommon> common;
        ASSERT_IS_OK(stream->getStreamCommon(&common));
        ASSERT_IS_OK(common->prepareToClose());
        ASSERT_IS_OK(common->close());
    }

    std::shared_ptr<ModuleBluetoothMock> module;
    std::unique_ptr<ModuleConfig> moduleConfig;
};

TEST_F(ModuleBluetoothTest, CreateOutputStream) {
    // 1. Find a Bluetooth Output Device Port (e.g. Out Headset)
    auto ports = moduleConfig->getExternalDevicePorts();
    auto portIt = std::find_if(ports.begin(), ports.end(), [](const auto& p) {
        return p.ext.template get<AudioPortExt::Tag::device>().device.type.type ==
               AudioDeviceType::OUT_HEADSET;
    });
    ASSERT_NE(portIt, ports.end()) << "No OUT_HEADSET port found";

    // 2. Connect External Device
    AudioPort connectedPort;
    ASSERT_NO_FATAL_FAILURE(connectExternalDevice(*portIt, &connectedPort));

    // 3. Configure Device Port
    auto devicePortConfig = moduleConfig->getSingleConfigForDevicePort(connectedPort);
    AudioPortConfig appliedDeviceConfig;
    bool applied = false;
    ASSERT_IS_OK(module->setAudioPortConfig(devicePortConfig, &appliedDeviceConfig, &applied));
    ASSERT_TRUE(applied);

    // 4. Find and Configure Mix Port (Output)
    auto mixPorts =
            moduleConfig->getRoutableMixPortsForDevicePort(connectedPort, true /*connectedOnly*/);
    ASSERT_FALSE(mixPorts.empty()) << "No routable mix ports found";
    auto mixPort = mixPorts[0];
    auto mixPortConfig = moduleConfig->getSingleConfigForMixPort(false /*isInput*/, mixPort);
    ASSERT_TRUE(mixPortConfig.has_value());
    const int mixPortHandle = 42;
    mixPortConfig->ext.get<AudioPortExt::mix>().handle = mixPortHandle;
    AudioPortConfig appliedMixConfig;
    ASSERT_IS_OK(module->setAudioPortConfig(*mixPortConfig, &appliedMixConfig, &applied));
    ASSERT_TRUE(applied);

    // 5. Create Patch
    AudioPatch patch;
    patch.sourcePortConfigIds = {appliedMixConfig.id};
    patch.sinkPortConfigIds = {appliedDeviceConfig.id};
    AudioPatch appliedPatch;
    ASSERT_IS_OK(module->setAudioPatch(patch, &appliedPatch));

    // 6. Open Stream
    OpenOutputStreamArguments args;
    args.portConfigId = appliedMixConfig.id;
    args.bufferSizeFrames = 1024;
    OpenOutputStreamReturn ret;
    ASSERT_IS_OK(module->openOutputStream(args, &ret));
    ASSERT_NE(nullptr, ret.stream);

    // 7. Critical check: getRecommendedLatencyModes must complete successfully because
    //    the mock proxy always returns `true`, thus a failure can only occur if the stream
    //    have not received a proxy.
    std::vector<AudioLatencyMode> modes;
    ASSERT_IS_OK(ret.stream->getRecommendedLatencyModes(&modes));
}

// See b/463287369.
TEST_F(ModuleBluetoothTest, ReproduceConfigReuseIssue) {
    // 1. Find a Bluetooth Output Device Port
    auto ports = moduleConfig->getExternalDevicePorts();
    auto portIt = std::find_if(ports.begin(), ports.end(), [](const auto& p) {
        return p.ext.template get<AudioPortExt::Tag::device>().device.type.type ==
               AudioDeviceType::OUT_HEADSET;
    });
    ASSERT_NE(portIt, ports.end()) << "No OUT_HEADSET port found";

    // 2. Connect External Device
    AudioPort connectedPort;
    ASSERT_NO_FATAL_FAILURE(connectExternalDevice(*portIt, &connectedPort));

    // 3. Configure Device Port (Initial Creation)
    auto devicePortConfig = moduleConfig->getSingleConfigForDevicePort(connectedPort);
    AudioPortConfig appliedDeviceConfig;
    bool applied = false;
    ASSERT_IS_OK(module->setAudioPortConfig(devicePortConfig, &appliedDeviceConfig, &applied));
    ASSERT_TRUE(applied);
    int32_t originalConfigId = appliedDeviceConfig.id;

    // 4. Configure Mix Port
    auto mixPorts = moduleConfig->getRoutableMixPortsForDevicePort(connectedPort, true);
    ASSERT_FALSE(mixPorts.empty());
    auto mixPort = mixPorts[0];
    auto mixPortConfig = moduleConfig->getSingleConfigForMixPort(false, mixPort);
    ASSERT_TRUE(mixPortConfig.has_value());
    const int mixPortHandle = 42;
    mixPortConfig->ext.get<AudioPortExt::mix>().handle = mixPortHandle;
    AudioPortConfig appliedMixConfig;
    ASSERT_IS_OK(module->setAudioPortConfig(*mixPortConfig, &appliedMixConfig, &applied));
    ASSERT_TRUE(applied);

    // 5. Create Patch (Simulate Stream 1)
    AudioPatch patch;
    patch.sourcePortConfigIds = {appliedMixConfig.id};
    patch.sinkPortConfigIds = {appliedDeviceConfig.id};
    AudioPatch appliedPatch;
    ASSERT_IS_OK(module->setAudioPatch(patch, &appliedPatch));

    // 6. Open Stream 1 (This consumes the Bluetooth Proxy)
    OpenOutputStreamArguments args;
    args.portConfigId = appliedMixConfig.id;
    args.bufferSizeFrames = 1024;
    std::shared_ptr<IStreamOut> stream1;
    {
        OpenOutputStreamReturn ret;
        ASSERT_IS_OK(module->openOutputStream(args, &ret));
        ASSERT_NE(nullptr, ret.stream);
        stream1 = ret.stream;
    }

    // 7. Close Stream 1 and Release Patch (This destroys the Bluetooth Proxy)
    closeStream(stream1);
    stream1.reset();
    ASSERT_IS_OK(module->resetAudioPatch(appliedPatch.id));

    // 8. REUSE: Try to configure the device port again using the SAME ID
    // This is the critical step. We re-send the previously applied config.
    // The module should find the existing config and reuse it.
    AudioPortConfig reuseConfig = appliedDeviceConfig;
    AudioPortConfig reusedResult;
    ASSERT_IS_OK(module->setAudioPortConfig(reuseConfig, &reusedResult, &applied));
    ASSERT_TRUE(applied);
    ASSERT_EQ(originalConfigId, reusedResult.id);

    // 9. Create New Patch (Simulate Stream 2)
    // This calls checkAudioPatchEndpointsMatch. If proxy is missing, it returns OK but
    // doesn't update connections map.
    AudioPatch newPatch = patch;
    AudioPatch newAppliedPatch;
    ASSERT_IS_OK(module->setAudioPatch(newPatch, &newAppliedPatch));

    // 10. Open Stream 2
    // This calls fetchAndCheckProxy. Fails to find connection/proxy. Creates stream with null
    // proxy.
    OpenOutputStreamReturn ret2;
    ASSERT_IS_OK(module->openOutputStream(args, &ret2));

    // 11. ASSERT FAILURE: Try to use the stream.
    // getRecommendedLatencyModes checks for the proxy and returns EX_ILLEGAL_STATE if missing.
    std::vector<AudioLatencyMode> modes;
    ASSERT_IS_OK(ret2.stream->getRecommendedLatencyModes(&modes));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    android::base::SetMinimumLogSeverity(::android::base::DEBUG);
    return RUN_ALL_TESTS();
}
