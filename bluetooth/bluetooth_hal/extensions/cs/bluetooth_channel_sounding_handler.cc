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

#define LOG_TAG "bluetooth_hal.extensions.cs"

#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_handler.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include "aidl/android/hardware/bluetooth/ranging/BluetoothChannelSoundingParameters.h"
#include "aidl/android/hardware/bluetooth/ranging/CsSecurityLevel.h"
#include "aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSession.h"
#include "aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSessionCallback.h"
#include "aidl/android/hardware/bluetooth/ranging/Reason.h"
#include "aidl/android/hardware/bluetooth/ranging/SessionType.h"
#include "aidl/android/hardware/bluetooth/ranging/VendorSpecificData.h"
#include "android-base/logging.h"
#include "android/binder_interface_utils.h"
#include "bluetooth_hal/config/cs_config_loader.h"
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_session_interface.h"
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_util.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_monitor.h"
#include "bluetooth_hal/hci_router.h"
#include "bluetooth_hal/util/android_base_wrapper.h"

#ifdef USE_RANGING_V1
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_session_v1.h"
#elif USE_RANGING_V2
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_session_v2.h"
#endif

namespace bluetooth_hal {
namespace extensions {
namespace cs {

namespace {

using ::aidl::android::hardware::bluetooth::ranging::
    BluetoothChannelSoundingParameters;
using ::aidl::android::hardware::bluetooth::ranging::CsSecurityLevel;
using ::aidl::android::hardware::bluetooth::ranging::
    IBluetoothChannelSoundingSession;
using ::aidl::android::hardware::bluetooth::ranging::
    IBluetoothChannelSoundingSessionCallback;
using ::aidl::android::hardware::bluetooth::ranging::Reason;
using ::aidl::android::hardware::bluetooth::ranging::SessionType;
using ::aidl::android::hardware::bluetooth::ranging::VendorSpecificData;
using ::bluetooth_hal::config::CsConfigLoader;

using ::bluetooth_hal::config::CsConfigLoader;
using ::bluetooth_hal::hci::EventCode;
using ::bluetooth_hal::hci::EventResultCode;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciBleMetaEventMonitor;
using ::bluetooth_hal::hci::HciConstants;
using ::bluetooth_hal::hci::HciMonitor;
using ::bluetooth_hal::hci::HciRouter;
using ::bluetooth_hal::hci::MonitorMode;
using ::bluetooth_hal::hci::MonitorType;
using ::bluetooth_hal::util::AndroidBaseWrapper;

using ::ndk::SharedRefBase;

struct RangingSettingCommand {
  uint8_t type = 0;
  uint8_t inline_pct = 0;
  uint32_t event_mask = 0;
  uint8_t mode_0_channel_map = 0;
};

// Factory function to create a session object based on the version.
std::shared_ptr<BluetoothChannelSoundingSessionInterface> CreateSession(
    [[maybe_unused]] std::shared_ptr<IBluetoothChannelSoundingSessionCallback>
        callback,
    [[maybe_unused]] Reason reason) {
#ifdef USE_RANGING_V1
  return SharedRefBase::make<BluetoothChannelSoundingSessionV1>(callback,
                                                                reason);
#elif USE_RANGING_V2
  return SharedRefBase::make<BluetoothChannelSoundingSessionV2>(callback,
                                                                reason);
#else
  return nullptr;
#endif
}

void SendFakeRasNotification(
    const BluetoothChannelSoundingParameters& parameters,
    int procedure_counter) {
  HalPacket packet = BuildRasNotification(parameters, procedure_counter);
  HciRouter::GetRouter().SendPacketToStack(packet);
}

RangingSettingCommand HandleOldData(const std::vector<uint8_t>& raw_data) {
  RangingSettingCommand data;

  data.type = raw_data[kOldIdxRangingSettingCommandType];
  data.inline_pct = raw_data[kOldIdxRangingSettingCommandInlinePct];
  data.event_mask = raw_data[kOldIdxRangingSettingCommandCSSubeventReport];
  data.mode_0_channel_map =
      raw_data[kOldIdxRangingSettingCommandMode0ChannelMap];

  return data;
}

RangingSettingCommand HandleData(const std::vector<uint8_t>& raw_data) {
  RangingSettingCommand data;

  int idx = -1;
  idx += kLenRangingSettingCommandType;
  data.type = raw_data[idx];

  idx += kLenRangingSettingCommandInlinePct;
  data.inline_pct = raw_data[idx];

  // Merge bytes into event_mask. For example, if raw_data is [..., 0x12, 0x34,
  // 0x56, 0x78, ...], event_mask will become 0x12345678.
  data.event_mask = 0;  // Initialize to ensure no garbage values
  for (int i = 0; i < kLenRangingSettingCommandEventMask; ++i) {
    idx += 1;
    data.event_mask |= static_cast<uint32_t>(raw_data[idx])
                       << (8 * (kLenRangingSettingCommandEventMask - 1 - i));
  }

  idx += kLenRangingSettingCommandMode0ChannelMap;
  data.mode_0_channel_map = raw_data[idx];

  return data;
}

bool IsBitSet(uint32_t value, int bitIndex) {
  return (bitIndex >= 0 && bitIndex <= 31) && ((value >> bitIndex) & 1);
}

}  // namespace

BluetoothChannelSoundingHandler::BluetoothChannelSoundingHandler()
    : cs_data_subevent_monitor_(kLeCsSubEventResultCode),
      cs_procedure_enable_subevent_monitor_(kLeCsProcedureEnableCompleteCode) {
  RegisterMonitor(cs_data_subevent_monitor_, MonitorMode::kMonitor);
  RegisterMonitor(cs_procedure_enable_subevent_monitor_, MonitorMode::kMonitor);
}

BluetoothChannelSoundingHandler::~BluetoothChannelSoundingHandler() {
  UnregisterMonitor(cs_data_subevent_monitor_);
  UnregisterMonitor(cs_procedure_enable_subevent_monitor_);
}

bool BluetoothChannelSoundingHandler::GetVendorSpecificData(
    std::optional<std::vector<std::optional<VendorSpecificData>>>*
        return_value) {
  // When the ranging HAL is bound, it first acquires vendor-specific data. Set
  // up all vendor-related components here.
  auto& cs_loader = CsConfigLoader::GetLoader();
  const std::vector<HalPacket>& calibration_commands =
      cs_loader.GetCsCalibrationCommands();

  if (calibration_commands.empty()) {
    LOG(WARNING) << __func__ << ": No calibration commands are found.";
  }

  LOG(INFO) << __func__ << ": Send calibration commands.";

  for (const auto& command : calibration_commands) {
    SendCommand(command);
  }

  *return_value =
      std::make_optional<std::vector<std::optional<VendorSpecificData>>>();

  if (local_capabilities_.size() <
      kCommandCompleteReadLocalCapabilityValueLength) {
    LOG(INFO) << __func__
              << ": Didn't get the current value. Return default value.";
    return true;
  }

  VendorSpecificData capability;
  capability.characteristicUuid = kUuidSpecialRangingSettingCapability;
  capability.opaqueValue = {kDataTypeData};
  for (int i = 0; i < kCommandCompleteReadLocalCapabilityValueLength; i++) {
    capability.opaqueValue.push_back(local_capabilities_[i]);
  }
  (*return_value)->push_back(capability);

  VendorSpecificData command;
  command.characteristicUuid = kUuidSpecialRangingSettingCommand;
  command.opaqueValue = {kDataTypeData};
  (*return_value)->push_back(command);

  return true;
}

bool BluetoothChannelSoundingHandler::GetSupportedSessionTypes(
    std::optional<std::vector<SessionType>>* return_value) {
  *return_value = {SessionType::SOFTWARE_STACK_DATA_PARSING};
  return true;
}

bool BluetoothChannelSoundingHandler::GetMaxSupportedCsSecurityLevel(
    CsSecurityLevel* return_value) {
  *return_value = CsSecurityLevel::ONE;
  return true;
}

bool BluetoothChannelSoundingHandler::OpenSession(
    const BluetoothChannelSoundingParameters& in_params,
    const std::shared_ptr<IBluetoothChannelSoundingSessionCallback>&
        in_callback,
    [[maybe_unused]] std::shared_ptr<IBluetoothChannelSoundingSession>*
        return_value) {
  if (in_params.vendorSpecificData.has_value()) {
    for (auto& data : in_params.vendorSpecificData.value()) {
      LOG(INFO) << "vendorSpecificData uuid:" << ToHex(data->characteristicUuid)
                << ", data:" << ToHex(data->opaqueValue);
    }
  }

  if (IsUuidMatched(in_params.vendorSpecificData) &&
      in_params.vendorSpecificData.value()[0].value().opaqueValue[0] ==
          kDataTypeReply) {
    HandleVendorSpecificReply(in_params.aclHandle, in_params.vendorSpecificData,
                              in_callback);
    return true;
  }

  auto session = CreateSession(in_callback, Reason::LOCAL_STACK_REQUEST);
  if (!session) {
    return false;
  }

  session->HandleVendorSpecificData(in_params.vendorSpecificData);
  SessionTracker tracker{.parameters = in_params};

  if (session->ShouldEnableFakeNotification()) {
    LOG(INFO) << __func__ << ": Enable fake notification.";
    tracker.is_fake_notification_enabled = true;
  }

  session_trackers_.insert_or_assign(in_params.aclHandle, tracker);

  if (session->ShouldEnableMode0ChannelMap()) {
    LOG(INFO) << __func__ << ": Enable mode 0 channel map.";
    HalPacket command = BuildEnableMode0ChannelMapCommand(
        static_cast<uint16_t>(in_params.aclHandle), kCommandValueEnable);
    SendCommand(command);
  }

#ifdef USE_RANGING_V1
  *return_value =
      std::static_pointer_cast<BluetoothChannelSoundingSessionV1>(session);
#elif USE_RANGING_V2
  *return_value =
      std::static_pointer_cast<BluetoothChannelSoundingSessionV2>(session);
#endif
  in_callback->onOpened(Reason::LOCAL_STACK_REQUEST);

  return true;
}

void BluetoothChannelSoundingHandler::HandleVendorSpecificReply(
    uint32_t connection_handle,
    const std::optional<std::vector<std::optional<VendorSpecificData>>>
        vendor_specific_data,
    const std::shared_ptr<IBluetoothChannelSoundingSessionCallback> callback) {
  LOG(INFO) << __func__ << ": connection_handle: 0x" << std::hex << std::setw(4)
            << std::setfill('0') << connection_handle;

  for (auto& data : vendor_specific_data.value()) {
    if (data.value().characteristicUuid != kUuidSpecialRangingSettingCommand) {
      continue;
    }

    LOG(INFO) << __func__ << ": Found command uuid for ranging setting.";

    bool is_old_format = false;
    RangingSettingCommand command_data;

    switch (data.value().opaqueValue.size()) {
      case kOldLengthDataFormat:
        command_data = HandleOldData(data.value().opaqueValue);
        is_old_format = true;
        break;
      case kLengthDataFormat:
        command_data = HandleData(data.value().opaqueValue);
        is_old_format = false;
        break;
      default:
        LOG(ERROR) << __func__ << ": Wrong length of the data format.";
        callback->onOpenFailed(Reason::LOCAL_STACK_REQUEST);
        return;
    }

    if (command_data.type != kDataTypeReply) {
      LOG(ERROR) << __func__ << ": Invalid data.";
      callback->onOpenFailed(Reason::LOCAL_STACK_REQUEST);
      return;
    }

    bool is_inline_pct_enabled = false;
    if (command_data.inline_pct != kCommandValueIgnore) {
      HalPacket command;
      switch (command_data.inline_pct) {
        case kCommandValueEnable:
          LOG(INFO) << __func__ << ": Send EnableInlinePctCommand.";
          command = BuildEnableInlinePctCommand(kCommandValueEnable);
          is_inline_pct_enabled = true;
          break;
        case kCommandValueDisable:
          LOG(INFO) << __func__ << ": Send DisableInlinePctCommand.";
          command = BuildEnableInlinePctCommand(kCommandValueDisable);
          is_inline_pct_enabled = false;
          break;
        default:
          LOG(ERROR) << __func__
                     << ": Invalid command value: " << command_data.inline_pct;
          callback->onOpenFailed(Reason::LOCAL_STACK_REQUEST);
          return;
      }
      SendCommand(command);
    }

    if (command_data.event_mask != kCommandValueIgnore) {
      HalPacket command;
      if (is_old_format) {
        switch (command_data.event_mask) {
          case kCommandValueEnable:
            LOG(INFO) << __func__
                      << ": Send EnableEventMaskForConnectionCommand.";
            command = BuildSetEventMaskForConnectionCommand(
                connection_handle,
                static_cast<uint32_t>(0b0111));  // Enable all 3 events.
            break;
          case kCommandValueDisable:
            LOG(INFO) << __func__
                      << ": Send DisableEventMaskForConnectionCommand.";
            command = BuildSetEventMaskForConnectionCommand(
                connection_handle,
                static_cast<uint32_t>(0b0000));  // Disable all 3 events.
            break;
          default:
            LOG(ERROR) << __func__ << ": Invalid command value: "
                       << command_data.event_mask;
            callback->onOpenFailed(Reason::LOCAL_STACK_REQUEST);
            return;
        }
      } else {
        command = BuildSetEventMaskForConnectionCommand(
            connection_handle, command_data.event_mask);
      }

      if (is_inline_pct_enabled) {
        bool enable_subevent_result_event =
            IsBitSet(command_data.event_mask, 0);
        bool enable_subevent_result_continue_event =
            IsBitSet(command_data.event_mask, 1);
        bool enable_procedure_enable_complete_event =
            IsBitSet(command_data.event_mask, 2);
        LOG(INFO)
            << __func__
            << ": Send SetEventMaskForConnectionCommand: Subevent Result event("
            << enable_subevent_result_event
            << "), Subevent Result Continue event("
            << enable_subevent_result_continue_event
            << "), Procedure Enable Complete event("
            << enable_procedure_enable_complete_event << ")";
        SendCommand(command);
      }
    }

    if (command_data.mode_0_channel_map != kCommandValueIgnore) {
      HalPacket command;
      switch (command_data.mode_0_channel_map) {
        case kCommandValueEnable:
          LOG(INFO) << __func__ << ": Send EnableMode0ChannelMapCommand.";

          command = BuildEnableMode0ChannelMapCommand(connection_handle,
                                                      kCommandValueEnable);
          break;
        case kCommandValueDisable:
          LOG(INFO) << __func__ << ": Send DisableMode0ChannelMapCommand.";
          command = BuildEnableMode0ChannelMapCommand(connection_handle,
                                                      kCommandValueDisable);
          break;
        default:
          LOG(ERROR) << __func__ << ": Invalid command value: "
                     << command_data.mode_0_channel_map;
          callback->onOpenFailed(Reason::LOCAL_STACK_REQUEST);
          return;
      }
      SendCommand(command);
    }
  }

  callback->onOpened(Reason::LOCAL_STACK_REQUEST);
}

void BluetoothChannelSoundingHandler::OnCommandCallback(
    const HalPacket& packet) {
  // Currently, two command types are supported:
  // 1) Calibration commands (opcode: 0xfd64).
  // 2) Ranging setting commands (opcode: 0xff0b).

  bool status = packet.GetCommandCompleteEventResult() ==
                static_cast<uint8_t>(EventResultCode::kSuccess);

  LOG(status ? INFO : WARNING)
      << __func__ << ": Recv VSE <" << packet.ToString() << "> "
      << (status ? "[Success]" : "[Failed]");

  if (!status ||
      packet.GetCommandOpcodeFromGeneratedEvent() !=
          kHciVscSpecialRangingSettingOpcode ||
      HciConstants::kHciCommandCompleteResultOffset + 1 >= packet.size()) {
    return;
  }

  uint8_t sub_opcode =
      packet[HciConstants::kHciCommandCompleteResultOffset + 1];

  // Store the read local cap value for Stack to read via
  // GetVendorSpecificData.
  if (sub_opcode == kHciVscReadLocalCapabilitySubOpCode) {
    if (packet.size() < kCommandCompleteReadLocalCapabilityOffset +
                            kCommandCompleteReadLocalCapabilityValueLength) {
      LOG(WARNING) << __func__ << ": Invalid event size.";
      return;
    }

    std::lock_guard<std::mutex> lock(local_cap_mtx_);

    local_capabilities_.clear();
    for (int i = 0; i < kCommandCompleteReadLocalCapabilityValueLength; i++) {
      local_capabilities_.push_back(
          packet[kCommandCompleteReadLocalCapabilityOffset + i]);
    }
  }
};

void BluetoothChannelSoundingHandler::HandleCsSubevent(
    const HalPacket& packet) {
  // [event_type (1 byte)] [event_code (1 byte)] [length (1 byte)]
  // [subevent_code (1 byte)] [connection_handle (2 bytes)].
  uint8_t offset = HciConstants::kHciBleEventSubCodeOffset + 1;
  uint16_t connection_handle =
      packet[offset] + ((packet[offset + 1] << 8u) & 0xff00);

  const auto tracker = GetTracker(connection_handle);
  if (!tracker || !tracker->get().is_fake_notification_enabled) {
    return;
  }

  // Skip config_id, start_acl_conn_event_counter.
  offset += 5;
  uint16_t procedure_counter =
      packet[offset] + ((packet[offset + 1] << 8u) & 0xff00);

  if (tracker->get().cur_procedure_counter == procedure_counter) {
    LOG(DEBUG) << __func__
               << ": Skip duplicate fake notification, procedure_counter: "
               << procedure_counter;
    return;
  }

  LOG(DEBUG) << __func__ << ": Send fake notification, connection_handle:"
             << connection_handle
             << ", procedure_counter:" << procedure_counter;

  tracker->get().cur_procedure_counter = procedure_counter;
  SendFakeRasNotification(tracker->get().parameters,
                          tracker->get().cur_procedure_counter);
}

void BluetoothChannelSoundingHandler::HandleCsProcedureEnableCompleteEvent(
    const HalPacket& packet) {
  // [event_type (1 byte)] [event_code (1 byte)] [length (1 byte)]
  // [subevent_code (1 byte)] [status (1 byte)][connection_handle (2
  // bytes)].
  uint8_t offset = HciConstants::kHciBleEventSubCodeOffset + 2;
  uint16_t connection_handle =
      packet[offset] + ((packet[offset + 1] << 8u) & 0xff00);

  const auto tracker = GetTracker(connection_handle);
  if (!tracker || !tracker->get().is_fake_notification_enabled) {
    return;
  }
  tracker->get().cur_procedure_counter = kInitialProcedureCounter;
}

void BluetoothChannelSoundingHandler::OnMonitorPacketCallback(
    [[maybe_unused]] MonitorMode mode, const HalPacket& packet) {
  uint8_t subevent_code = packet.GetBleSubEventCode();
  switch (subevent_code) {
    case kLeCsSubEventResultCode:
      HandleCsSubevent(packet);
      break;
    case kLeCsProcedureEnableCompleteCode:
      HandleCsProcedureEnableCompleteEvent(packet);
      break;
    default:
      break;
  }
};

std::optional<
    std::reference_wrapper<BluetoothChannelSoundingHandler::SessionTracker>>
BluetoothChannelSoundingHandler::GetTracker(uint16_t connection_handle) {
  auto it = session_trackers_.find(connection_handle);
  if (it == session_trackers_.end()) {
    return std::nullopt;
  }
  return it->second;
}

void BluetoothChannelSoundingHandler::OnBluetoothEnabled() {
  SendCommand(BuildReadLocalCapabilityCommand());
};

void BluetoothChannelSoundingHandler::OnBluetoothDisabled() {
  std::lock_guard<std::mutex> lock(local_cap_mtx_);
  local_capabilities_.clear();
};

}  // namespace cs
}  // namespace extensions
}  // namespace bluetooth_hal
