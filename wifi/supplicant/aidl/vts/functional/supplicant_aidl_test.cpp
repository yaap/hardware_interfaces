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
#include <android/binder_manager.h>
#include <android/binder_status.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <cutils/properties.h>

#include "supplicant_test_utils.h"
#include "wifi_aidl_test_utils.h"

using aidl::android::hardware::wifi::supplicant::DebugLevel;
using aidl::android::hardware::wifi::supplicant::IfaceInfo;
using aidl::android::hardware::wifi::supplicant::IfaceType;
using android::ProcessState;

class SupplicantAidlTest : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        initializeService();
        supplicant_ = getSupplicant(GetParam().c_str());
        ASSERT_NE(supplicant_, nullptr);
        ASSERT_TRUE(supplicant_->setDebugParams(DebugLevel::EXCESSIVE, true, true).isOk());
    }

    void TearDown() override {
        stopSupplicantService();
        startWifiFramework();
    }

  protected:
    std::shared_ptr<ISupplicant> supplicant_;
};

/*
 * GetDebugLevel
 */
TEST_P(SupplicantAidlTest, GetDebugLevel) {
    DebugLevel retrievedLevel;
    DebugLevel expectedLevel = DebugLevel::WARNING;
    ASSERT_TRUE(supplicant_->setDebugParams(expectedLevel, true, true).isOk());
    ASSERT_TRUE(supplicant_->getDebugLevel(&retrievedLevel).isOk());
    ASSERT_EQ(retrievedLevel, expectedLevel);
}

/*
 * ListAndRemoveInterface
 */
TEST_P(SupplicantAidlTest, ListAndRemoveInterface) {
    // Ensure that the STA interface exists
    std::shared_ptr<ISupplicantStaIface> sta_iface;
    EXPECT_TRUE(supplicant_->getStaInterface(getStaIfaceName(), &sta_iface).isOk());
    ASSERT_NE(sta_iface, nullptr);

    // Interface list should contain at least 1 interface
    std::vector<IfaceInfo> ifaces;
    EXPECT_TRUE(supplicant_->listInterfaces(&ifaces).isOk());
    ASSERT_FALSE(ifaces.empty());
    int prevNumIfaces = ifaces.size();

    // Remove an interface and verify that it is removed from the list
    EXPECT_TRUE(supplicant_->removeInterface(ifaces[0]).isOk());
    EXPECT_TRUE(supplicant_->listInterfaces(&ifaces).isOk());
    ASSERT_NE(ifaces.size(), prevNumIfaces);
}

/*
 * SetConcurrencyPriority
 */
TEST_P(SupplicantAidlTest, SetConcurrencyPriority) {
    // Valid values
    ASSERT_TRUE(supplicant_->setConcurrencyPriority(IfaceType::STA).isOk());
    ASSERT_TRUE(supplicant_->setConcurrencyPriority(IfaceType::P2P).isOk());

    // Invalid value
    ASSERT_FALSE(supplicant_->setConcurrencyPriority(static_cast<IfaceType>(2)).isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SupplicantAidlTest);
INSTANTIATE_TEST_SUITE_P(
        Supplicant, SupplicantAidlTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(ISupplicant::descriptor)),
        android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
