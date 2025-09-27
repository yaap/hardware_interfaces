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

#ifdef USE_EXT_V1

#define LOG_TAG "bluetooth_hal.extensions.ext"

#include "bluetooth_hal/extensions/ext/bluetooth_ext.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "android-base/logging.h"
#include "android/binder_auto_utils.h"
#include "android/binder_interface_utils.h"
#include "android/binder_manager.h"
#include "bluetooth_hal/extensions/ext/bluetooth_ext_handler.h"
#include "bluetooth_hal/hal_extension_points.h"

namespace bluetooth_hal {
namespace extensions {
namespace ext {
namespace {

using ::bluetooth_hal::extensions::BluetoothHalRegisterExtension;

using ::ndk::ICInterface;
using ::ndk::ScopedAStatus;
using ::ndk::SharedRefBase;

void ExtInitializer() {
  auto register_service = [](const std::shared_ptr<ICInterface>& service,
                             const char* name) {
    std::string instance = std::string() + name + "/default";
    binder_status_t status =
        AServiceManager_addService(service->asBinder().get(), instance.c_str());
    if (status != STATUS_OK) {
      LOG(ERROR) << "Could not register " << name << " as a service!";
    }
  };

  register_service(SharedRefBase::make<BluetoothExt>(),
                   BluetoothExt::descriptor);
}

}  // namespace

struct ExtRegistrar {
  ExtRegistrar() { BluetoothHalRegisterExtension(ExtInitializer); }
};

ExtRegistrar g_ext_registrar;

ScopedAStatus BluetoothExt::setBluetoothCmdPacket(
    char16_t opcode, const std::vector<uint8_t>& params, bool* ret) {
  bool status = handler_.SetBluetoothCmdPacket(opcode, params, ret);
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
}

}  // namespace ext
}  // namespace extensions
}  // namespace bluetooth_hal

#endif
