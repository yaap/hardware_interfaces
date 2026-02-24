/*
 * Copyright (C) 2026 The Android Open Source Project
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

#define LOG_TAG "VtsHalAudioProduct"
#include <android-base/logging.h>

#include <android-base/properties.h>
#include <gtest/gtest.h>

// @VsrTest = GMS-VSR-5.3.14-014
TEST(AudioProductTest, BluetoothScoManagerByAudioEnabled) {
    // See 'property_initialize_ro_vendor_api_level' in system/core/init/property_service.cpp
    auto apiLevel = ::android::base::GetIntProperty<int32_t>("ro.vendor.api_level", 0);
    if (apiLevel < 202604) {
        GTEST_SKIP() << "Skipping test as the vendor level is below 202604: " << apiLevel;
    }
    EXPECT_TRUE(::android::base::GetBoolProperty("bluetooth.sco.managed_by_audio", false))
            << "DEVICEs launching with Android 17 or higher and with CHIPSETs that set "
            << "\'ro.board.first_api_level\' or \'ro.board.api_level\' to 202604 or higher "
            << "MUST set Bluetooth property bluetooth.sco.managed_by_audio to true.";
}
