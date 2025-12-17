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

#ifdef USE_CHANNEL_AVOIDANCE_V1

#define LOG_TAG "bluetooth_hal.extensions.channel_avoidance"

#include "bluetooth_hal/extensions/channel_avoidance/bluetooth_channel_avoidance.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>

#include "android-base/logging.h"
#include "android/binder_auto_utils.h"
#include "android/binder_interface_utils.h"
#include "android/binder_manager.h"
#include "bluetooth_hal/extensions/channel_avoidance/bluetooth_channel_avoidance_handler.h"
#include "bluetooth_hal/hal_extension_points.h"

namespace bluetooth_hal::extensions::channel_avoidance {
namespace {

using ::bluetooth_hal::extensions::BluetoothHalRegisterExtension;

using ::ndk::ICInterface;
using ::ndk::ScopedAStatus;
using ::ndk::SharedRefBase;

void ChannelAvoidanceInitializer() {
  RegisterHalService(SharedRefBase::make<BluetoothChannelAvoidance>());
}

}  // namespace

struct ChannelAvoidanceRegistrar {
  ChannelAvoidanceRegistrar() {
    BluetoothHalRegisterExtension(ChannelAvoidanceInitializer);
  }
};

ChannelAvoidanceRegistrar g_channel_avoidance_registrar;

ScopedAStatus BluetoothChannelAvoidance::setBluetoothChannelStatus(
    const std::array<uint8_t, 10>& channel_map) {
  bool status = handler_.SetBluetoothChannelStatus(channel_map);
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
}

}  // namespace bluetooth_hal::extensions::channel_avoidance

#endif
