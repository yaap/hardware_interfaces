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

#define LOG_TAG "npu_aidl_hal_test"

#include <cassert>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/npu/BnSchedulingCallback.h>
#include <aidl/android/hardware/npu/IScheduling.h>
#include <aidl/android/hardware/npu/SchedulingConfig.h>
#include <aidl/android/hardware/npu/WorkInfo.h>
#include <android-base/properties.h>
#include <android/binder_auto_utils.h>
#include <android/binder_enums.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/content/pm/IPackageManagerNative.h>
#include <binder/IServiceManager.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using android::getAidlHalInstanceNames;
using android::PrintInstanceNameToString;
using ndk::enum_range;
using ndk::ScopedAStatus;
using ndk::SharedRefBase;
using ndk::SpAIBinder;
using testing::AllOf;
using testing::AnyOf;
using testing::AnyOfArray;
using testing::AssertionFailure;
using testing::AssertionResult;
using testing::AssertionSuccess;
using testing::Contains;
using testing::Each;
using testing::Eq;
using testing::ExplainMatchResult;
using testing::Ge;
using testing::Gt;
using testing::Le;
using testing::Lt;
using testing::Matcher;
using testing::Not;
using namespace std::string_literals;
using namespace std::chrono_literals;

const std::string FEATURE_HARDWARE_NPU = "android.hardware.npu";

namespace aidl::android::hardware::npu {

class NpuSchedulingAidl : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
        scheduling = IScheduling::fromBinder(binder);
        ASSERT_NE(scheduling, nullptr);
    }
    std::shared_ptr<IScheduling> scheduling;
};

class SchedulingCallback : public BnSchedulingCallback {
  public:
    SchedulingCallback() = default;
    ::ndk::ScopedAStatus onIdle() override { return ndk::ScopedAStatus::ok(); }

    ::ndk::ScopedAStatus onWorkRequested([[maybe_unused]] const WorkInfo& workInfo) override {
        return ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus onWorkStarted([[maybe_unused]] const WorkInfo& workInfo,
                                       [[maybe_unused]] const StartReason reason) override {
        return ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus onWorkEnded([[maybe_unused]] const WorkInfo& workInfo,
                                     [[maybe_unused]] const EndReason reason) override {
        return ndk::ScopedAStatus::ok();
    }
};

/*
 * Tests that setSchedulingConfigs() works with valid input
 */
TEST_P(NpuSchedulingAidl, SetSchedulingConfigs) {
    std::vector<SchedulingConfig> configs;

    SchedulingConfig config;
    config.uid = 1001;
    config.priority = 500;
    config.hasDirectAccess = true;
    config.canAttributeOtherUid = false;
    configs.push_back(config);

    auto status = scheduling->setSchedulingConfigs(configs);
    ASSERT_TRUE(status.isOk()) << "setSchedulingConfigs failed: " << status.getDescription();
}

/*
 * Tests that setSchedulingConfigs() rejects invalid priorities
 */
TEST_P(NpuSchedulingAidl, SetSchedulingConfigsInvalidPriority) {
    std::vector<SchedulingConfig> configs;

    SchedulingConfig config;
    config.uid = 1001;
    config.priority = -100;
    config.hasDirectAccess = true;
    config.canAttributeOtherUid = false;
    configs.push_back(config);

    auto status = scheduling->setSchedulingConfigs(configs);
    ASSERT_FALSE(status.isOk())
            << "setSchedulingConfigs with invalid priority must return EX_ILLEGAL_ARGUMENT";
    ASSERT_EQ(status.getExceptionCode(), EX_ILLEGAL_ARGUMENT);
}

/*
 * Tests that updateSchedulingConfigs() works with valid input
 */
TEST_P(NpuSchedulingAidl, UpdateSchedulingConfigs) {
    std::vector<SchedulingConfig> configs;

    SchedulingConfig config;
    config.uid = 1001;
    config.priority = 500;
    config.hasDirectAccess = true;
    config.canAttributeOtherUid = false;
    configs.push_back(config);

    auto status = scheduling->updateSchedulingConfigs(configs);
    ASSERT_TRUE(status.isOk()) << "updateSchedulingConfigs failed: " << status.getDescription();
}

/*
 * Tests that setCallback() works with valid instance
 */
TEST_P(NpuSchedulingAidl, SetCallback) {
    auto callback = ndk::SharedRefBase::make<SchedulingCallback>();

    auto status = scheduling->setCallback(callback);
    ASSERT_TRUE(status.isOk()) << "setCallback failed: " << status.getDescription();
}

/*
 * Tests that setCallaback() works with a null instance
 */
TEST_P(NpuSchedulingAidl, SetCallbackNull) {
    auto status = scheduling->setCallback(nullptr);
    ASSERT_TRUE(status.isOk()) << "setCallback failed: " << status.getDescription();
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(NpuSchedulingAidl);
INSTANTIATE_TEST_SUITE_P(NpuScheduling, NpuSchedulingAidl,
                         testing::ValuesIn(getAidlHalInstanceNames(IScheduling::descriptor)),
                         PrintInstanceNameToString);

// Check whether the given named feature is available.
static bool checkFeature(const std::string& name) {
    ::android::sp<::android::IServiceManager> sm(::android::defaultServiceManager());
    ::android::sp<::android::IBinder> binder(
            sm->waitForService(::android::String16("package_native")));
    if (binder == nullptr) {
        GTEST_LOG_(ERROR) << "waitForService package_native failed";
        return false;
    }
    ::android::sp<::android::content::pm::IPackageManagerNative> packageMgr =
            ::android::interface_cast<::android::content::pm::IPackageManagerNative>(binder);
    if (packageMgr == nullptr) {
        GTEST_LOG_(ERROR) << "Cannot find package manager";
        return false;
    }
    bool hasFeature = false;
    auto status = packageMgr->hasSystemFeature(::android::String16(name.c_str()), 0, &hasFeature);
    if (!status.isOk()) {
        GTEST_LOG_(ERROR) << "hasSystemFeature('" << name << "') failed: " << status;
        return false;
    }
    return hasFeature;
}

// [VSR-5.7-001] (if device has an NPU as indicated by FEATURE_HARDWARE_NPU) MUST support
// FEATURE_NPU and implement the android.hardware.npu HAL interface
TEST(NpuFeature, ImplementsHal) {
    if (!checkFeature(FEATURE_HARDWARE_NPU)) {
        GTEST_SKIP() << "Device does not declare feature " << FEATURE_HARDWARE_NPU;
        return;
    }

    ASSERT_FALSE(getAidlHalInstanceNames(IScheduling::descriptor).empty())
            << "No implementation for " << IScheduling::descriptor << " found";
}

}  // namespace aidl::android::hardware::npu

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
