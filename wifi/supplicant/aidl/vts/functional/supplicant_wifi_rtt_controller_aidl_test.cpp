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

#include <VtsCoreUtil.h>
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/wifi/supplicant/BnSupplicant.h>
#include <aidl/android/hardware/wifi/supplicant/BnSupplicantWifiRttController.h>
#include <aidl/android/hardware/wifi/supplicant/BnSupplicantWifiRttControllerEventCallback.h>
#include <android/binder_manager.h>
#include <android/binder_status.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <cutils/properties.h>

#include "supplicant_test_utils.h"
#include "wifi_aidl_test_utils.h"

using aidl::android::hardware::wifi::supplicant::BnSupplicantWifiRttControllerEventCallback;
using aidl::android::hardware::wifi::supplicant::DebugLevel;
using aidl::android::hardware::wifi::supplicant::ISupplicant;
using aidl::android::hardware::wifi::supplicant::ISupplicantStaIface;
using aidl::android::hardware::wifi::supplicant::ISupplicantWifiRttController;
using aidl::android::hardware::wifi::supplicant::ISupplicantWifiRttControllerEventCallback;
using aidl::android::hardware::wifi::supplicant::MacAddress;
using aidl::android::hardware::wifi::supplicant::RttCapabilities;
using aidl::android::hardware::wifi::supplicant::RttConfig;
using aidl::android::hardware::wifi::supplicant::RttResult;
using android::ProcessState;

class SupplicantWifiRttControllerEventCallback : public BnSupplicantWifiRttControllerEventCallback {
  public:
    SupplicantWifiRttControllerEventCallback() = default;

    ::ndk::ScopedAStatus onResults(int /* cmdId */,
                                   const std::vector<RttResult>& /* results */) override {
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus onContinuousRangingStatusChanged(
            int /* cmdId */,
            BnSupplicantWifiRttControllerEventCallback::ContinuousRangingStatusCode /* code */)
            override {
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus onContinuousRangingTerminated(
            int /* cmdId */, BnSupplicantWifiRttControllerEventCallback::
                                     ContinuousRangingTerminateReasonCode /* reason */) override {
        return ::ndk::ScopedAStatus::ok();
    }
};

class SupplicantRttControllerAidlTest : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        if (!::testing::deviceSupportsFeature("android.hardware.wifi")) {
            GTEST_SKIP() << "Skipping this test since wifi is not supported.";
        }
        initializeService();
        supplicant_ = getSupplicant(GetParam().c_str());
        ASSERT_NE(supplicant_, nullptr);
        ASSERT_TRUE(supplicant_->getInterfaceVersion(&interface_version_).isOk());
        if (interface_version_ < 5) {
            GTEST_SKIP() << "SupplicantRttControllerAidlTest is available as of Supplicant V5";
        }
        ASSERT_TRUE(supplicant_
                            ->setDebugParams(DebugLevel::EXCESSIVE,
                                             true,  // show timestamps
                                             true)
                            .isOk());
        EXPECT_TRUE(supplicant_->getStaInterface(getStaIfaceName(), &sta_iface_).isOk());
        ASSERT_NE(sta_iface_, nullptr);
        EXPECT_TRUE(supplicant_->createRttController(getStaIfaceName(), &rtt_controller_).isOk());
        ASSERT_NE(rtt_controller_, nullptr);
    }

    void TearDown() override {
        stopSupplicantService();
        startWifiFramework();
    }

  protected:
    std::shared_ptr<ISupplicant> supplicant_;
    std::shared_ptr<ISupplicantStaIface> sta_iface_;
    std::shared_ptr<ISupplicantWifiRttController> rtt_controller_;
    int interface_version_;
};

/*
 * GetName
 */
TEST_P(SupplicantRttControllerAidlTest, GetName) {
    std::string iface_name;
    auto status = rtt_controller_->getName(&iface_name);
    ASSERT_TRUE(status.isOk());
    EXPECT_EQ(iface_name, getStaIfaceName());
}

/*
 * GetCapabilities
 */
TEST_P(SupplicantRttControllerAidlTest, GetCapabilities) {
    RttCapabilities capabilities;
    auto status = rtt_controller_->getCapabilities(&capabilities);
    ASSERT_TRUE(status.isOk());
}

/*
 * SetProximityRangingDeviceName
 */
TEST_P(SupplicantRttControllerAidlTest, SetProximityRangingDeviceName) {
    // TODO set device name and call GetCapabilities to confirm
    const std::string name = "TestDeviceName";
    auto status = rtt_controller_->setProximityRangingDeviceName(name);
    ASSERT_TRUE(status.isOk());
}

/*
 * SetProximityRangingMacAddress
 */
TEST_P(SupplicantRttControllerAidlTest, SetProximityRangingMacAddress) {
    const std::array<uint8_t, 6> macAddress = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    auto status = rtt_controller_->setProximityRangingMacAddress(macAddress);
    ASSERT_TRUE(status.isOk());
}

/*
 * GetProximityRangingMacAddress
 */
TEST_P(SupplicantRttControllerAidlTest, GetProximityRangingMacAddress) {
    // TODO combine SetProximityRangingMacAddress and GetProximityRangingMacAddress
    std::array<uint8_t, 6> macAddress;
    auto status = rtt_controller_->getProximityRangingMacAddress(&macAddress);
    ASSERT_TRUE(status.isOk());
}

/*
 * RangeRequest
 */
TEST_P(SupplicantRttControllerAidlTest, RangeRequest) {
    int32_t cmdId = 123;
    std::vector<RttConfig> rttConfigs;
    RttConfig config;
    config.addr = {0x01, 0x02, 0x03, 0x04, 0x05, 0x07};
    rttConfigs.push_back(config);
    auto status = rtt_controller_->rangeRequest(cmdId, rttConfigs);
    ASSERT_TRUE(status.isOk());
}

/*
 * RangeCancel
 */
TEST_P(SupplicantRttControllerAidlTest, RangeCancel) {
    int32_t cmdId = 123;
    MacAddress peerAddr = {{0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc}};
    std::vector peerAddrList = {peerAddr};
    auto status = rtt_controller_->rangeCancel(cmdId, peerAddrList);
    ASSERT_TRUE(status.isOk());
}

/*
 * RegisterEventCallback
 */
TEST_P(SupplicantRttControllerAidlTest, RegisterEventCallback) {
    std::shared_ptr<SupplicantWifiRttControllerEventCallback> callback =
            ndk::SharedRefBase::make<SupplicantWifiRttControllerEventCallback>();
    ASSERT_NE(callback, nullptr);
    EXPECT_TRUE(rtt_controller_->registerEventCallback(callback).isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SupplicantRttControllerAidlTest);
INSTANTIATE_TEST_SUITE_P(
        Supplicant, SupplicantRttControllerAidlTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(ISupplicant::descriptor)),
        android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
