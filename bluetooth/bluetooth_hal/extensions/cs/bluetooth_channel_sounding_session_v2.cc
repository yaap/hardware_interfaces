/*
 * Copyright 2025 The Android Open Source Project
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

#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_session_v2.h"

#include <memory>
#include <optional>
#include <vector>

#include "aidl/android/hardware/bluetooth/ranging/ChannelSoudingRawData.h"
#include "aidl/android/hardware/bluetooth/ranging/ChannelSoundingProcedureData.h"
#include "aidl/android/hardware/bluetooth/ranging/Config.h"
#include "aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSessionCallback.h"
#include "aidl/android/hardware/bluetooth/ranging/ProcedureEnableConfig.h"
#include "aidl/android/hardware/bluetooth/ranging/Reason.h"
#include "aidl/android/hardware/bluetooth/ranging/ResultType.h"
#include "aidl/android/hardware/bluetooth/ranging/VendorSpecificData.h"
#include "android/binder_auto_utils.h"
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_distance_estimator.h"
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_distance_estimator_interface.h"
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_session_interface.h"

namespace bluetooth_hal {
namespace extensions {
namespace cs {

using ::aidl::android::hardware::bluetooth::ranging::ChannelSoudingRawData;
using ::aidl::android::hardware::bluetooth::ranging::
    ChannelSoundingProcedureData;
using ::aidl::android::hardware::bluetooth::ranging::Config;
using ::aidl::android::hardware::bluetooth::ranging::
    IBluetoothChannelSoundingSessionCallback;
using ::aidl::android::hardware::bluetooth::ranging::ProcedureEnableConfig;
using ::aidl::android::hardware::bluetooth::ranging::Reason;
using ::aidl::android::hardware::bluetooth::ranging::ResultType;
using ::aidl::android::hardware::bluetooth::ranging::VendorSpecificData;
using ::ndk::ScopedAStatus;

BluetoothChannelSoundingSessionV2::BluetoothChannelSoundingSessionV2(
    std::shared_ptr<IBluetoothChannelSoundingSessionCallback> callback,
    Reason /* reason */)
    : distance_estimator_(ChannelSoundingDistanceEstimatorInterface::Create()) {
  callback_ = callback;
}

ScopedAStatus BluetoothChannelSoundingSessionV2::getVendorSpecificReplies(
    [[maybe_unused]] std::optional<
        std::vector<std::optional<VendorSpecificData>>>* _aidl_return) {
  return ScopedAStatus::ok();
};

ScopedAStatus BluetoothChannelSoundingSessionV2::getSupportedResultTypes(
    [[maybe_unused]] std::vector<ResultType>* _aidl_return) {
  return ScopedAStatus::ok();
};

ScopedAStatus BluetoothChannelSoundingSessionV2::isAbortedProcedureRequired(
    [[maybe_unused]] bool* _aidl_return) {
  return ScopedAStatus::ok();
};

ScopedAStatus BluetoothChannelSoundingSessionV2::writeProcedureData(
    [[maybe_unused]] const ChannelSoundingProcedureData& in_procedureData) {
  return ScopedAStatus::ok();
};

ScopedAStatus writeRawData(
    [[maybe_unused]] const ChannelSoudingRawData& in_rawData) {
  return ScopedAStatus::ok();
}

ScopedAStatus BluetoothChannelSoundingSessionV2::close(
    [[maybe_unused]] Reason in_reason) {
  return ScopedAStatus::ok();
};

ScopedAStatus BluetoothChannelSoundingSessionV2::updateChannelSoundingConfig(
    [[maybe_unused]] const Config& in_config) {
  return ScopedAStatus::ok();
};

ScopedAStatus BluetoothChannelSoundingSessionV2::updateProcedureEnableConfig(
    [[maybe_unused]] const ProcedureEnableConfig& in_procedureEnableConfig) {
  return ScopedAStatus::ok();
};

ScopedAStatus BluetoothChannelSoundingSessionV2::updateBleConnInterval(
    [[maybe_unused]] int in_bleConnInterval) {
  return ScopedAStatus::ok();
};

void BluetoothChannelSoundingSessionV2::HandleVendorSpecificData(
    [[maybe_unused]] const std::optional<
        std::vector<std::optional<VendorSpecificData>>>
        vendor_specific_data) {};

bool BluetoothChannelSoundingSessionV2::ShouldEnableFakeNotification() {
  return false;
};

bool BluetoothChannelSoundingSessionV2::ShouldEnableMode0ChannelMap() {
  return false;
};

}  // namespace cs
}  // namespace extensions
}  // namespace bluetooth_hal
