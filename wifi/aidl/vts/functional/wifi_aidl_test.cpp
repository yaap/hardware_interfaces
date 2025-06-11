/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Staache License, Version 2.0 (the "License");
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

#include <vector>

#include <VtsCoreUtil.h>
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/wifi/BnWifiEventCallback.h>
#include <android/binder_manager.h>
#include <android/binder_status.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include "wifi_aidl_test_utils.h"

using aidl::android::hardware::wifi::BnWifiEventCallback;
using aidl::android::hardware::wifi::WifiStatusCode;

class WifiAidlTest : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        instance_name_ = GetParam().c_str();
        stopWifiService(instance_name_);
        wifi_ = getWifi(instance_name_);
        ASSERT_NE(wifi_, nullptr);
    }

    void TearDown() override { stopWifiService(instance_name_); }

  protected:
    std::shared_ptr<IWifi> wifi_;
    const char* instance_name_;
};

class WifiEventCallback : public BnWifiEventCallback {
  public:
    WifiEventCallback() = default;

    ::ndk::ScopedAStatus onFailure(WifiStatusCode /* status */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onStart() override { return ndk::ScopedAStatus::ok(); }
    ::ndk::ScopedAStatus onStop() override { return ndk::ScopedAStatus::ok(); }
    ::ndk::ScopedAStatus onSubsystemRestart(WifiStatusCode /* status */) override {
        return ndk::ScopedAStatus::ok();
    }
};

/*
 * RegisterEventCallback
 */
TEST_P(WifiAidlTest, RegisterEventCallback) {
    const std::shared_ptr<WifiEventCallback> callback =
            ndk::SharedRefBase::make<WifiEventCallback>();
    ASSERT_NE(callback, nullptr);
    EXPECT_TRUE(wifi_->registerEventCallback(callback).isOk());
}

/*
 * IsStarted
 */
TEST_P(WifiAidlTest, IsStarted) {
    // HAL should not be started by default
    bool isStarted;
    EXPECT_TRUE(wifi_->isStarted(&isStarted).isOk());
    EXPECT_FALSE(isStarted);

    // Start wifi by setting up the chip, and verify isStarted
    auto wifiChip = getWifiChip(instance_name_);
    EXPECT_NE(wifiChip, nullptr);
    EXPECT_TRUE(wifi_->isStarted(&isStarted).isOk());
    EXPECT_TRUE(isStarted);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(WifiAidlTest);
INSTANTIATE_TEST_SUITE_P(WifiTest, WifiAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IWifi::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    android::ProcessState::self()->setThreadPoolMaxThreadCount(1);
    android::ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
