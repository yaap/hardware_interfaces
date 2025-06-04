/*
 * Copyright 2024 The Android Open Source Project
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

#define LOG_TAG "bthal.router_client"

#include "bluetooth_hal/hci_router_client.h"

#include <algorithm>
#include <functional>
#include <map>
#include <mutex>

#include "android-base/logging.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_monitor.h"
#include "bluetooth_hal/hci_router.h"

namespace bluetooth_hal {
namespace hci {

using ::bluetooth_hal::HalState;

HciRouterClient::HciRouterClient() {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  current_state_ = HalState::kShutdown;
  HciRouter::GetRouter().RegisterCallback(this);
}

HciRouterClient::~HciRouterClient() {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  monitors_.clear();
  HciRouter::GetRouter().UnregisterCallback(this);
}

MonitorMode HciRouterClient::OnPacketCallback(const HalPacket& packet) {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  if (!IsBluetoothEnabled()) {
    // Look for HCI_RESET complete event if Bluetooth is not enabled.
    HandleBluetoothEnable(packet);
  }

  // Find the mode with the highest priority.
  MonitorMode mode = MonitorMode::kNone;
  for (const auto& it : monitors_) {
    if (it.first == packet) {
      mode = (it.second > mode) ? it.second : mode;
    }
  }

  if (mode != MonitorMode::kNone) {
    OnMonitorPacketCallback(mode, packet);
  }
  return mode;
}

bool HciRouterClient::IsBluetoothChipReady() {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  return is_bluetooth_chip_ready_;
}

bool HciRouterClient::IsBluetoothEnabled() {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  return is_bluetooth_enabled_;
}

bool HciRouterClient::RegisterMonitor(const HciMonitor& monitor,
                                      MonitorMode mode) {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  if (mode == MonitorMode::kNone) {
    LOG(ERROR) << __func__ << ": Monitor mode cannot be kNone!";
    return false;
  }
  auto it = std::find_if(
      monitors_.begin(), monitors_.end(),
      [&monitor](const auto& entry) { return entry.first == monitor; });
  if (it != monitors_.end()) {
    LOG(ERROR) << __func__ << ": The same monitor already exist!";
    return false;
  }
  monitors_.insert({monitor, mode});
  return true;
}

bool HciRouterClient::UnregisterMonitor(const HciMonitor& monitor) {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  auto it = std::find_if(
      monitors_.begin(), monitors_.end(),
      [&monitor](const auto& entry) { return entry.first == monitor; });
  if (it == monitors_.end()) {
    LOG(ERROR) << __func__ << ": Monitor not registered!";
    return false;
  }
  monitors_.erase(it);
  return true;
}

bool HciRouterClient::SendCommand(const HalPacket& packet) {
  // TODO: Add debug message for the client has sent a command.
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  if (packet.GetType() != HciPacketType::kCommand) {
    return false;
  }
  return HciRouter::GetRouter().SendCommand(
      packet, std::bind_front(&HciRouterClient::OnCommandCallback, this));
}

bool HciRouterClient::SendData(const HalPacket& packet) {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  if (packet.GetType() == HciPacketType::kCommand) {
    return false;
  }
  return HciRouter::GetRouter().Send(packet);
}

void HciRouterClient::OnHalStateChanged(HalState new_state,
                                        HalState old_state) {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);

  if (current_state_ > old_state) {
    LOG(WARNING) << __func__
                 << " (old_state, current_state_in_client) is mismatched! "
                    "[ old_state("
                 << static_cast<int>(old_state) << ") -> new_state("
                 << static_cast<int>(new_state)
                 << ") ], current_state_in_client: "
                 << static_cast<int>(current_state_);
    return;
  }

  current_state_ = new_state;

  switch (new_state) {
    case HalState::kBtChipReady:
      if (!is_bluetooth_chip_ready_) {
        OnBluetoothChipReady();
      }
      if (is_bluetooth_enabled_) {
        OnBluetoothDisabled();
      }
      is_bluetooth_chip_ready_ = true;
      is_bluetooth_enabled_ = false;
      break;
    case HalState::kRunning:
      if (!is_bluetooth_chip_ready_) {
        OnBluetoothChipReady();
      }
      // We do not handle is_bluetooth_enabled_ here because the clients have to
      // wait for a HCI_RESET before they can send packets to the chip.
      is_bluetooth_chip_ready_ = true;
      break;
    default:
      if (is_bluetooth_chip_ready_) {
        OnBluetoothChipClosed();
      }
      if (is_bluetooth_enabled_) {
        OnBluetoothDisabled();
      }
      is_bluetooth_chip_ready_ = false;
      is_bluetooth_enabled_ = false;
      break;
  }
}

void HciRouterClient::HandleBluetoothEnable(const HalPacket& packet) {
  if (HciRouter::GetRouter().GetHalState() == HalState::kRunning &&
      packet.GetCommandOpcodeFromGeneratedEvent() ==
          static_cast<uint16_t>(CommandOpCode::kHciReset) &&
      packet.GetCommandCompleteEventResult() ==
          static_cast<uint8_t>(EventResultCode::kSuccess)) {
    // Inform the client that Bluetooth has enabled after a HCI_RESET command is
    // sent in kRunning state.
    is_bluetooth_enabled_ = true;
    OnBluetoothEnabled();
  }
}

}  // namespace hci
}  // namespace bluetooth_hal
