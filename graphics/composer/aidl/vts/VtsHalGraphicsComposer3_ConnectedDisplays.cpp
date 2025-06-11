/**
 * Copyright (c) 2025, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/graphics/common/BlendMode.h>
#include <aidl/android/hardware/graphics/common/BufferUsage.h>
#include <aidl/android/hardware/graphics/common/FRect.h>
#include <aidl/android/hardware/graphics/common/PixelFormat.h>
#include <aidl/android/hardware/graphics/common/Rect.h>
#include <aidl/android/hardware/graphics/composer3/Composition.h>
#include <aidl/android/hardware/graphics/composer3/IComposer.h>
#include <android-base/properties.h>
#include <android/binder_process.h>
#include <android/hardware/graphics/composer3/ComposerClientReader.h>
#include <android/hardware/graphics/composer3/ComposerClientWriter.h>
#include <binder/ProcessState.h>
#include <gtest/gtest.h>
#include <ui/Fence.h>
#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>
#include <string>
#include <unordered_map>
#include "ComposerClientWrapper.h"

#undef LOG_TAG
#define LOG_TAG "VtsHalGraphicsComposer3_ConnectedDisplays"

namespace aidl::android::hardware::graphics::composer3::vts {

using namespace std::chrono_literals;
using namespace aidl::android::hardware::graphics::composer3::libhwc_aidl_test;

/**
 * @class ConnectedDisplaysTest
 * @brief A test suite for validating the HWC (Hardware Composer) API when multiple displays are
 * present.
 *
 * This test suite is part of the VTS (Vendor Test Suite) and is designed to test the interactions
 * between multiple displays using the HWC API. It ensures that the API behaves correctly when more
 * than one display are present.
 *
 * @note The test requires at least two displays to be found. If only one display is found, the test
 * will be skipped.
 */
class ConnectedDisplaysTest : public ::testing::TestWithParam<std::string> {
  protected:
    void SetUp() override {
        mComposerClient = std::make_unique<ComposerClientWrapper>(GetParam());
        ASSERT_TRUE(mComposerClient->createClient().isOk());

        const auto& [status, displays] = mComposerClient->getDisplays();
        ASSERT_TRUE(status.isOk());
        mDisplays = displays;

        // Skip test if there's only one display
        if (mDisplays.size() <= 1) {
            GTEST_SKIP() << "Test requires at least 2 displays, found " << mDisplays.size();
        }

        // explicitly disable vsync for all displays
        for (const auto& display : mDisplays) {
            EXPECT_TRUE(mComposerClient->setVsync(display.getDisplayId(), false).isOk());
        }
        mComposerClient->setVsyncAllowed(false);
    }

    void TearDown() override {
        ASSERT_TRUE(
                mComposerClient->tearDown(std::unordered_map<int64_t, ComposerClientWriter*>{}));
        mComposerClient.reset();
    }

    std::unique_ptr<ComposerClientWrapper> mComposerClient;
    std::vector<DisplayWrapper> mDisplays;
    static constexpr uint32_t kBufferSlotCount = 64;
};

/**
 * Test that verifies display configurations can be changed independently without affecting other
 * displays.
 */
TEST_P(ConnectedDisplaysTest, IndependentConfigChange) {
    // Store initial configs for all displays
    std::unordered_map<int64_t, int32_t> initialConfigs;
    for (const auto& display : mDisplays) {
        const auto& [activeStatus, activeConfig] =
                mComposerClient->getActiveConfig(display.getDisplayId());
        ASSERT_TRUE(activeStatus.isOk());
        initialConfigs[display.getDisplayId()] = activeConfig;
    }

    for (auto& display : mDisplays) {
        const auto& [status, configs] = mComposerClient->getDisplayConfigs(display.getDisplayId());
        ASSERT_TRUE(status.isOk());
        ASSERT_FALSE(configs.empty());

        // Try to set each config
        for (const auto& config : configs) {
            if (config == initialConfigs[display.getDisplayId()]) continue;

            EXPECT_TRUE(mComposerClient->setActiveConfig(&display, config).isOk());

            // Verify other displays' configs remain unchanged
            for (const auto& otherDisplay : mDisplays) {
                if (otherDisplay.getDisplayId() != display.getDisplayId()) {
                    const auto& [otherStatus, otherConfig] =
                            mComposerClient->getActiveConfig(otherDisplay.getDisplayId());
                    EXPECT_TRUE(otherStatus.isOk());
                    EXPECT_EQ(otherConfig, initialConfigs[otherDisplay.getDisplayId()]);
                }
            }
        }
        // Restore original config
        EXPECT_TRUE(
                mComposerClient->setActiveConfig(&display, initialConfigs[display.getDisplayId()])
                        .isOk());
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ConnectedDisplaysTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, ConnectedDisplaysTest,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IComposer::descriptor)),
        ::android::PrintInstanceNameToString);

}  // namespace aidl::android::hardware::graphics::composer3::vts
