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

#define LOG_TAG "bluetooth_hal.extensions.cs.v2"

#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_v2.h"

#include <memory>
#include <optional>
#include <vector>

#include "aidl/android/hardware/bluetooth/ranging/BluetoothChannelSoundingParameters.h"
#include "aidl/android/hardware/bluetooth/ranging/CsSecurityLevel.h"
#include "aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSession.h"
#include "aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSessionCallback.h"
#include "aidl/android/hardware/bluetooth/ranging/SessionType.h"
#include "aidl/android/hardware/bluetooth/ranging/VendorSpecificData.h"
#include "android-base/logging.h"
#include "android/binder_auto_utils.h"
#include "android/binder_interface_utils.h"
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_handler.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace extensions {
namespace cs {

using ::ndk::ScopedAStatus;

using ::aidl::android::hardware::bluetooth::ranging::
    BluetoothChannelSoundingParameters;
using ::aidl::android::hardware::bluetooth::ranging::CsSecurityLevel;
using ::aidl::android::hardware::bluetooth::ranging::
    IBluetoothChannelSoundingSession;
using ::aidl::android::hardware::bluetooth::ranging::SessionType;
using ::aidl::android::hardware::bluetooth::ranging::VendorSpecificData;

using ::aidl::android::hardware::bluetooth::ranging::
    IBluetoothChannelSoundingSessionCallback;

using ::ndk::ScopedAStatus;
using ::ndk::SharedRefBase;

ScopedAStatus BluetoothChannelSoundingV2::getVendorSpecificData(
    std::optional<std::vector<std::optional<VendorSpecificData>>>*
        _aidl_return) {
  bool status =
      bluetooth_channel_sounding_handler_.GetVendorSpecificData(_aidl_return);
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
}

ScopedAStatus BluetoothChannelSoundingV2::getSupportedSessionTypes(
    std::optional<std::vector<SessionType>>* _aidl_return) {
  bool status = bluetooth_channel_sounding_handler_.GetSupportedSessionTypes(
      _aidl_return);
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
}

ScopedAStatus BluetoothChannelSoundingV2::getMaxSupportedCsSecurityLevel(
    [[maybe_unused]] CsSecurityLevel* _aidl_return) {
  bool status =
      bluetooth_channel_sounding_handler_.GetMaxSupportedCsSecurityLevel(
          _aidl_return);
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
};

ScopedAStatus BluetoothChannelSoundingV2::openSession(
    [[maybe_unused]] const BluetoothChannelSoundingParameters& in_params,
    [[maybe_unused]] const std::shared_ptr<
        IBluetoothChannelSoundingSessionCallback>& in_callback,
    [[maybe_unused]] std::shared_ptr<IBluetoothChannelSoundingSession>*
        _aidl_return) {
  LOG(INFO) << __func__;

  if (in_callback.get() == nullptr) {
    return ScopedAStatus::fromExceptionCodeWithMessage(
        EX_ILLEGAL_ARGUMENT, "Invalid nullptr callback");
  }

  bool status = bluetooth_channel_sounding_handler_.OpenSession(
      in_params, in_callback, _aidl_return);
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
}

ScopedAStatus BluetoothChannelSoundingV2::getSupportedCsSecurityLevels(
    [[maybe_unused]] std::vector<CsSecurityLevel>* _aidl_return) {
  return ScopedAStatus::ok();
};

}  // namespace cs
}  // namespace extensions
}  // namespace bluetooth_hal
