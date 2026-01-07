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

#define LOG_TAG "bluetooth_hal.bt_activities"

#include "bluetooth_hal/debug/bluetooth_activities.h"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

#include "bluetooth_hal/bluetooth_address.h"
#include "bluetooth_hal/debug/command_error_code.h"
#include "bluetooth_hal/debug/debug_client.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_monitor.h"
#include "bluetooth_hal/hci_router_client.h"
#include "bluetooth_hal/util/logging.h"
#include "com_android_bluetooth_bluetooth_hal_flags.h"

namespace bluetooth_hal::debug {
namespace {

namespace hal_flags = ::com::android::bluetooth::bluetooth_hal::flags;

using ::bluetooth_hal::hci::BleMetaEventSubCode;
using ::bluetooth_hal::hci::BluetoothAddress;
using ::bluetooth_hal::hci::EventCode;
using ::bluetooth_hal::hci::EventResultCode;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciBleMetaEventMonitor;
using ::bluetooth_hal::hci::HciEventMonitor;
using ::bluetooth_hal::hci::MonitorMode;
using ::bluetooth_hal::util::Logger;

constexpr std::string_view kBluetoothActivitiesDebuggingTitle =
    "Bluetooth Activities";
constexpr uint16_t kBtMaxConnectHistoryRecord = 64;
constexpr size_t kBleConnectionEventStatusOffset = 4;
constexpr size_t kBleConnectionHandleOffset = 5;
constexpr size_t kBleConnectionBdAddressOffset = 9;
constexpr size_t kConnectionEventStatusOffset = 3;
constexpr size_t kConnectionHandleOffset = 4;
constexpr size_t kConnectionBdAddressOffset = 6;
constexpr size_t kDisconnectionEventStatusOffset = 3;
constexpr size_t kDisconnectionHandleOffset = 4;
constexpr int kUint8HexStringDigit = 2;
constexpr int kUint16HexStringDigit = 4;

std::string ToHexString(uint16_t value, int num_of_digits) {
  std::stringstream ss;
  ss << std::hex << std::setw(num_of_digits) << std::setfill('0') << value;
  return "0x" + ss.str();
}

}  // namespace

std::unique_ptr<BluetoothActivities> BluetoothActivities::instance_ = nullptr;

class BluetoothActivitiesImpl : public BluetoothActivities,
                                public ::bluetooth_hal::hci::HciRouterClient,
                                public DebugClient {
 public:
  BluetoothActivitiesImpl();

  bool HasConnectedDevice() const;
  bool IsConnected(uint16_t connection_handle) const;
  size_t GetConnectionHandleCount() const;
  void HandleBleMetaEvent(const ::bluetooth_hal::hci::HalPacket& event);
  void HandleConnectCompleteEvent(const ::bluetooth_hal::hci::HalPacket& event);
  void HandleDisconnectCompleteEvent(
      const ::bluetooth_hal::hci::HalPacket& event);

  void OnCommandCallback(
      [[maybe_unused]] const ::bluetooth_hal::hci::HalPacket& packet) override {
  };
  void OnMonitorPacketCallback(
      ::bluetooth_hal::hci::MonitorMode mode,
      const ::bluetooth_hal::hci::HalPacket& packet) override;
  void OnBluetoothChipReady() override;
  void OnBluetoothChipClosed() override;
  void OnBluetoothEnabled() override {};
  void OnBluetoothDisabled() override {};

  ConnectionCallbackSubscription RegisterConnectionCountChangedCallback(
      ConnectionCountChangedCallback callback) override;

#ifndef UNIT_TEST
  std::vector<Coredump> Dump() override;
#endif

 private:
  struct ConnectionActivity {
    uint16_t connection_handle;
    BluetoothAddress bd_address;
    std::string event;
    std::string status;
    std::string timestamp;
  };

  void OnDeviceConnected(const BluetoothAddress& bd_address,
                         uint16_t connection_handle);
  void OnDeviceDisconnected(uint16_t connection_handle);
  void OnAllDevicesDisconnected();
  void UnregisterConnectionCountChangedCallback(uint32_t id);

  void UpdateConnectionHistory(const ConnectionActivity& device);

  HciBleMetaEventMonitor ble_connection_complete_event_monitor_;
  HciBleMetaEventMonitor ble_enhanced_connection_complete_v1_event_monitor_;
  HciBleMetaEventMonitor ble_enhanced_connection_complete_v2_event_monitor_;
  HciEventMonitor connection_complete_event_monitor_;
  HciEventMonitor disconnection_complete_event_monitor_;

  std::list<ConnectionActivity> connection_history_;
  std::unordered_map<uint16_t, BluetoothAddress> connected_device_address_;
  std::unordered_map<uint32_t, ConnectionCountChangedCallback>
      connection_count_changed_callbacks_;
  uint32_t next_callback_id_ = 0;
};

BluetoothActivitiesImpl::BluetoothActivitiesImpl()
    : ble_connection_complete_event_monitor_(HciBleMetaEventMonitor(
          static_cast<uint8_t>(BleMetaEventSubCode::kConnectionComplete))),
      ble_enhanced_connection_complete_v1_event_monitor_(
          HciBleMetaEventMonitor(static_cast<uint8_t>(
              BleMetaEventSubCode::kEnhancedConnectionCompleteV1))),
      ble_enhanced_connection_complete_v2_event_monitor_(
          HciBleMetaEventMonitor(static_cast<uint8_t>(
              BleMetaEventSubCode::kEnhancedConnectionCompleteV2))),
      connection_complete_event_monitor_(HciEventMonitor(
          static_cast<uint8_t>(EventCode::kConnectionComplete))),
      disconnection_complete_event_monitor_(HciEventMonitor(
          static_cast<uint8_t>(EventCode::kDisconnectionComplete))) {
  SetClientLogTag(kBluetoothActivitiesDebuggingTitle.data());

  RegisterMonitor(ble_connection_complete_event_monitor_,
                  MonitorMode::kMonitor);
  RegisterMonitor(connection_complete_event_monitor_, MonitorMode::kMonitor);
  RegisterMonitor(disconnection_complete_event_monitor_, MonitorMode::kMonitor);
}

void BluetoothActivities::Start() { BluetoothActivities::Get(); }

BluetoothActivities& BluetoothActivities::Get() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!instance_) {
    instance_ = std::make_unique<BluetoothActivitiesImpl>();
  }
  return *instance_;
}

void BluetoothActivities::Stop() {
  if (!instance_) {
    return;
  }
  instance_.reset();
}

bool BluetoothActivitiesImpl::HasConnectedDevice() const {
  return connected_device_address_.size() > 0;
}

bool BluetoothActivitiesImpl::IsConnected(uint16_t connection_handle) const {
  return connected_device_address_.find(connection_handle) !=
         connected_device_address_.end();
}

size_t BluetoothActivitiesImpl::GetConnectionHandleCount() const {
  return connected_device_address_.size();
}

void BluetoothActivitiesImpl::OnMonitorPacketCallback(
    [[maybe_unused]] MonitorMode mode, const HalPacket& packet) {
  switch (packet.GetEventCode()) {
    case static_cast<uint8_t>(EventCode::kBleMeta):
      HandleBleMetaEvent(packet);
      break;
    case static_cast<uint8_t>(EventCode::kConnectionComplete):
      HandleConnectCompleteEvent(packet);
      break;
    case static_cast<uint8_t>(EventCode::kDisconnectionComplete):
      HandleDisconnectCompleteEvent(packet);
      break;
  }
}

void BluetoothActivitiesImpl::OnBluetoothChipReady() {
  CLIENT_LOG(INFO) << __func__;
}

void BluetoothActivitiesImpl::OnBluetoothChipClosed() {
  OnAllDevicesDisconnected();
}

BluetoothActivities::ConnectionCallbackSubscription
BluetoothActivitiesImpl::RegisterConnectionCountChangedCallback(
    ConnectionCountChangedCallback callback) {
  if (!hal_flags::bt_activities_subscription()) {
    return ConnectionCallbackSubscription([] {});
  }

  uint32_t id = next_callback_id_++;
  connection_count_changed_callbacks_[id] = std::move(callback);
  return ConnectionCallbackSubscription(
      [this, id] { UnregisterConnectionCountChangedCallback(id); });
}

void BluetoothActivitiesImpl::UnregisterConnectionCountChangedCallback(
    uint32_t id) {
  if (!hal_flags::bt_activities_subscription()) {
    return;
  }

  connection_count_changed_callbacks_.erase(id);
}

void BluetoothActivitiesImpl::HandleBleMetaEvent(const HalPacket& event) {
  uint8_t event_status = event.At(kBleConnectionEventStatusOffset);
  ConnectionActivity activity{
      .connection_handle =
          event.AtUint16LittleEndian(kBleConnectionHandleOffset),
      .bd_address = event.GetBluetoothAddressAt(kBleConnectionBdAddressOffset),
      .event = "LE Connection Complete " +
               ToHexString(event.GetBleSubEventCode(), kUint8HexStringDigit),
      .status = std::string(GetResultString(event_status)),
      .timestamp = Logger::GetLogFormatTimestamp(),
  };
  UpdateConnectionHistory(activity);

  if (event_status == static_cast<uint8_t>(EventResultCode::kSuccess)) {
    OnDeviceConnected(activity.bd_address, activity.connection_handle);
    CLIENT_LOG(INFO) << __func__ << ": " << activity.event
                     << ", connection handle: "
                     << ToHexString(activity.connection_handle,
                                    kUint16HexStringDigit)
                     << ", BD address: " << activity.bd_address.ToString()
                     << ".";
  }
}

void BluetoothActivitiesImpl::HandleConnectCompleteEvent(
    const HalPacket& event) {
  uint8_t event_status = event.At(kConnectionEventStatusOffset);
  ConnectionActivity activity{
      .connection_handle = event.AtUint16LittleEndian(kConnectionHandleOffset),
      .bd_address = event.GetBluetoothAddressAt(kConnectionBdAddressOffset),
      .event = "Connect Complete " +
               ToHexString(event.GetEventCode(), kUint8HexStringDigit),
      .status = std::string(GetResultString(event_status)),
      .timestamp = Logger::GetLogFormatTimestamp(),
  };
  UpdateConnectionHistory(activity);

  if (event_status == static_cast<uint8_t>(EventResultCode::kSuccess)) {
    OnDeviceConnected(activity.bd_address, activity.connection_handle);
    CLIENT_LOG(INFO) << __func__ << ": " << activity.event
                     << ", connection handle: "
                     << ToHexString(activity.connection_handle,
                                    kUint16HexStringDigit)
                     << ", BD address: " << activity.bd_address.ToString()
                     << ".";
  }
}

void BluetoothActivitiesImpl::HandleDisconnectCompleteEvent(
    const HalPacket& event) {
  uint8_t event_status = event.At(kDisconnectionEventStatusOffset);
  uint16_t connection_handle =
      event.AtUint16LittleEndian(kDisconnectionHandleOffset);
  ConnectionActivity activity{
      .connection_handle = connection_handle,
      .bd_address = connected_device_address_[connection_handle],
      .event = "Disconnect Complete " +
               ToHexString(event.GetEventCode(), kUint8HexStringDigit),
      .status = std::string(GetResultString(event_status)),
      .timestamp = Logger::GetLogFormatTimestamp(),
  };
  UpdateConnectionHistory(activity);

  if (event_status == static_cast<uint8_t>(EventResultCode::kSuccess)) {
    OnDeviceDisconnected(activity.connection_handle);
    CLIENT_LOG(INFO) << __func__ << ": " << activity.event
                     << ", connection handle: "
                     << ToHexString(activity.connection_handle,
                                    kUint16HexStringDigit)
                     << ", BD address: " << activity.bd_address.ToString()
                     << ".";
  }
}

void BluetoothActivitiesImpl::OnDeviceConnected(
    const BluetoothAddress& bd_address, uint16_t connection_handle) {
  connected_device_address_[connection_handle] = bd_address;

  if (hal_flags::bt_activities_subscription()) {
    for (const auto& [_, callback] : connection_count_changed_callbacks_) {
      callback(connected_device_address_.size());
    }
  }
}

void BluetoothActivitiesImpl::OnDeviceDisconnected(uint16_t connection_handle) {
  connected_device_address_.erase(connection_handle);

  if (hal_flags::bt_activities_subscription()) {
    for (const auto& [_, callback] : connection_count_changed_callbacks_) {
      callback(connected_device_address_.size());
    }
  }
}

void BluetoothActivitiesImpl::OnAllDevicesDisconnected() {
  connected_device_address_.clear();
  CLIENT_LOG(INFO) << __func__ << ": " << "Clear connected devices.";

  if (hal_flags::bt_activities_subscription()) {
    for (const auto& [_, callback] : connection_count_changed_callbacks_) {
      callback(0);
    }
  }
}

void BluetoothActivitiesImpl::UpdateConnectionHistory(
    const ConnectionActivity& device) {
  if (connection_history_.size() >= kBtMaxConnectHistoryRecord) {
    connection_history_.pop_front();
  }
  connection_history_.emplace_back(device);
}

#ifndef UNIT_TEST
std::vector<Coredump> BluetoothActivitiesImpl::Dump() {
  if (!hal_flags::coredump_bt_activities()) {
    return std::vector<Coredump>();
  }

  std::string connection_history_dump_;
  for (const ConnectionActivity& activity : connection_history_) {
    connection_history_dump_ +=
        activity.timestamp + ": " + activity.event + ", connection handle: " +
        ToHexString(activity.connection_handle, kUint16HexStringDigit) +
        ", BD address: " + activity.bd_address.ToString() +
        ", status: " + activity.status + "\n";
  }

  std::vector<Coredump> bluetooth_activities_coredumps;
  bluetooth_activities_coredumps.emplace_back(
      kBluetoothActivitiesDebuggingTitle.data(), connection_history_dump_,
      CoredumpPosition::kEnd);

  return bluetooth_activities_coredumps;
}
#endif

}  // namespace bluetooth_hal::debug
