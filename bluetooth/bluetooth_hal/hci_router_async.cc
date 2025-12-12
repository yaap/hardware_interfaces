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

#define LOG_TAG "bluetooth_hal.hci_router_async"

#include "bluetooth_hal/hci_router_async.h"

#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>

#include "bluetooth_hal/chip/async_chip_provisioner.h"
#include "bluetooth_hal/config/hal_config_loader.h"
#include "bluetooth_hal/debug/debug_central.h"
#include "bluetooth_hal/debug/vnd_snoop_logger.h"
#include "bluetooth_hal/extensions/thread/thread_handler.h"
#include "bluetooth_hal/hci_monitor.h"
#include "bluetooth_hal/hci_router_client_agent.h"
#include "bluetooth_hal/transport/transport_interface.h"
#include "bluetooth_hal/util/logging.h"
#include "bluetooth_hal/util/power/wakelock.h"
#include "bluetooth_hal/util/worker.h"

namespace bluetooth_hal::hci {

using ::bluetooth_hal::HalState;
using ::bluetooth_hal::chip::AsyncChipProvisioner;
using ::bluetooth_hal::config::HalConfigLoader;
using ::bluetooth_hal::debug::VndSnoopLogger;
using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::thread::ThreadHandler;
using ::bluetooth_hal::transport::TransportInterface;
using ::bluetooth_hal::transport::TransportInterfaceCallback;
using ::bluetooth_hal::util::Worker;
using ::bluetooth_hal::util::power::ScopedWakelock;
using ::bluetooth_hal::util::power::Wakelock;
using ::bluetooth_hal::util::power::WakeSource;

/*
 * kHalStateMachine contains the sequence of the HciRouter state machine.
 * The Shutdown state, BtChipReady state and Running state are static states.
 * The state machine stays in the Shutdown state if the Bluetooth chip is
 * powered off. The state machine stays in the BtChipReady state if the
 * controller is fully ready, including Bluetooth is off when the "Accelerate BT
 * ON" feature is enabled. The state machine stays in the Running state after
 * the Bluetooth stack sends the first HCI_RESET command, indicating the
 * Bluetooth process is ready.
 *
 * All states can switch to the Shutdown state for error handling.
 *
 *                         ╔═══╗
 *                         ║   v
 *          ╔═══════════ kShutdown <═══════════╦══════════════════╗
 *          ║               ^                  ║                  ║
 *          v               ║                  ║                  ║
 *        kInit ════════════╣             kBtChipReady <════> kRunning
 *          ║               ║                  ^
 *          v               ║                  ║
 *  kPreFirmwareDownload════║                  ║
 *          ║               ║                  ║
 *          v               ║                  ║
 *  kFirmwareDownloading════╬══════════ kFirmwareReady
 *          ║               ║                  ^
 *          ║               ║                  ║
 *          ╚══> kFirmwareDownloadCompleted ═══╝
 *
 * Format of the map: {CurrentState, {ValidNextState1, ValidNextState2, ...}}
 */
static const std::unordered_map<HalState, std::unordered_set<HalState>>
    kHalStateMachine = {
        {HalState::kShutdown, {HalState::kShutdown, HalState::kInit}},
        {HalState::kInit,
         {HalState::kShutdown, HalState::kPreFirmwareDownload}},
        {HalState::kPreFirmwareDownload,
         {HalState::kShutdown, HalState::kFirmwareDownloading}},
        {HalState::kFirmwareDownloading,
         {HalState::kShutdown, HalState::kFirmwareDownloadCompleted}},
        {HalState::kFirmwareDownloadCompleted,
         {HalState::kShutdown, HalState::kFirmwareReady}},
        {HalState::kFirmwareReady,
         {HalState::kShutdown, HalState::kBtChipReady}},
        {HalState::kBtChipReady,
         {HalState::kShutdown, HalState::kBtChipReady, HalState::kRunning}},
        {HalState::kRunning, {HalState::kShutdown, HalState::kBtChipReady}},
};

HciRouterAsync::HciRouterAsync()
    : Worker(std::bind_front(&HciRouterAsync::TaskHandler, this)) {}

bool HciRouterAsync::DoInRouterThread(std::function<void()> task) {
  if (Worker::Post(RouterTask(task))) {
    VoteRouterTaskWakelock();
    return true;
  }
  return false;
}

bool HciRouterAsync::SynchronousDoInRouterThread(std::function<void()> task) {
  std::promise<void> promise;
  auto future = promise.get_future();
  bool status = DoInRouterThread([&promise, task = std::move(task)]() {
    task();
    promise.set_value();
  });

  if (status) {
    future.wait();
  }
  return status;
}

void HciRouterAsync::TaskHandler(RouterTask task) {
  SCOPED_ANCHOR(AnchorType::kRouterTask, __func__);
  HAL_LOG(VERBOSE) << "HciRouterAsync: handling RouterTask";
  task.Run();
  UnvoteRouterTaskWakelock();
}

void HciRouterAsync::VoteRouterTaskWakelock() {
  // This method is called by the thread that dispatches a task to the router
  // thread.
  std::unique_lock<std::mutex> lock(task_wakelock_mutex_);
  if (wake_lock_votes_ == 0) {
    Wakelock::GetWakelock().Acquire(WakeSource::kRouterTask);
  }
  wake_lock_votes_++;
}

void HciRouterAsync::UnvoteRouterTaskWakelock() {
  // This method is called by the router thread when a task is completed.
  std::unique_lock<std::mutex> lock(task_wakelock_mutex_);
  Wakelock::GetWakelock().Release(WakeSource::kRouterTask);
  wake_lock_votes_--;
  if (wake_lock_votes_ > 0) {
    Wakelock::GetWakelock().Acquire(WakeSource::kRouterTask);
  }
}

bool HciRouterAsync::Initialize(
    const std::shared_ptr<HciRouterCallback>& callback,
    TransportInterfaceCallback* transport_callback) {
  HAL_LOG(INFO) << "Initializing Bluetooth HCI Router.";
  hci_callback_ = callback;
  return InitializeModules(transport_callback);
}

bool HciRouterAsync::InitializeModules(
    TransportInterfaceCallback* transport_callback) {
  transport_callback_ = transport_callback;
  switch (hal_state_) {
    case HalState::kRunning:
      LOG(WARNING) << "HciRouter has already initialized!";
      return false;
    case HalState::kShutdown:
      break;
    case HalState::kBtChipReady:
      if (HalConfigLoader::GetLoader().IsAcceleratedBtOnSupported()) {
#ifndef UNIT_TEST
        AsyncChipProvisioner::GetProvisioner().PostResetFirmware();
#endif
        return true;
      }
      [[fallthrough]];
    default:
      LOG(WARNING) << "HciRouter is initializing!";
      return true;
  }

  HciRouterAsync::UpdateHalState(HalState::kInit);

  if (!InitializeTransport()) {
    LOG(ERROR) << "Failed to initialize transport!";
    Close();
    return false;
  }

  LOG(INFO) << "Start downloading Bluetooth firmware.";
#ifndef UNIT_TEST
  AsyncChipProvisioner::GetProvisioner().PostInitialize(
      [this](HalState hal_state) {
        SynchronousDoInRouterThread(
            [this, hal_state]() { UpdateHalState(hal_state); });
      });
  AsyncChipProvisioner::GetProvisioner().PostDownloadFirmware();
#endif

  return true;
}

void HciRouterAsync::Close() {
  if (hal_state_ == HalState::kRunning &&
      HalConfigLoader::GetLoader().IsAcceleratedBtOnSupported()) {
#ifndef UNIT_TEST
    AsyncChipProvisioner::GetProvisioner().PostResetFirmware();
#endif
    return;
  }
  HciRouterAsync::Cleanup();
}

void HciRouterAsync::Cleanup() {
  HAL_LOG(INFO) << "Shutting down the HciRouter";

  SetBusy(false);
  std::queue<QueuedHciCommand> empty;
  std::swap(hci_cmd_queue_, empty);

  if (ThreadHandler::IsHandlerRunning()) {
    ThreadHandler::Cleanup();
  }

  TransportInterface::CleanupTransport();

  HciRouterAsync::UpdateHalState(HalState::kShutdown);
  hci_callback_ = nullptr;
  transport_callback_ = nullptr;
}

bool HciRouterAsync::Send(const HalPacket& packet) {
  packet.SetDestination(PacketDestination::kController);

  if (packet.GetType() == HciPacketType::kCommand) {
    return HciRouterAsync::SendCommand(
        packet, std::make_shared<HalPacketCallback>(std::bind_front(
                    &HciRouterCallback::OnCommandCallback, hci_callback_)));
  }

  if (HciRouterClientAgent::GetAgent().DispatchPacketToClients(packet) ==
      MonitorMode::kIntercept) {
    HAL_LOG(DEBUG) << __func__ << ": packet intercepted by a client, "
                   << packet.ToString();
    return true;
  }
  return SendToTransport(packet);
}

bool HciRouterAsync::SendCommand(
    const HalPacket& packet,
    const std::shared_ptr<HalPacketCallback>& callback) {
  packet.SetDestination(PacketDestination::kController);

  if (packet.GetCommandOpcode() ==
      static_cast<uint16_t>(CommandOpCode::kGoogleDebugInfo)) {
    return HciRouterAsync::SendCommandNoAck(packet);
  }

  if (HciRouterClientAgent::GetAgent().DispatchPacketToClients(packet) ==
      MonitorMode::kIntercept) {
    HAL_LOG(DEBUG) << __func__ << ": packet intercepted by a client, "
                   << packet.ToString();
    return true;
  }
  return SendOrQueueCommand(packet, callback);
}

bool HciRouterAsync::SendCommandNoAck(const HalPacket& packet) {
  packet.SetDestination(PacketDestination::kController);
  if (HciRouterClientAgent::GetAgent().DispatchPacketToClients(packet) ==
      MonitorMode::kIntercept) {
    HAL_LOG(DEBUG) << __func__ << ": packet intercepted by a client, "
                   << packet.ToString();
    return true;
  }
  return SendToTransport(packet);
}

HalState HciRouterAsync::GetHalState() { return hal_state_; }

void HciRouterAsync::UpdateHalState(HalState state) {
  std::stringstream ss;
  ss << HalStateToString(hal_state_) << " (" << static_cast<int>(hal_state_)
     << ") -> " << HalStateToString(state) << " (" << static_cast<int>(state)
     << ")";
  HAL_LOG(INFO) << "Bluetooth HAL state changed: " << ss.str();
  if (!IsHalStateValid(state)) {
    LOG(FATAL) << "Invalid Bluetooth HAL state changed! " << ss.str();
  }

  auto old_state = hal_state_;
  hal_state_ = state;
  std::shared_ptr<void> defer_task;

  switch (state) {
    case HalState::kShutdown:
      VndSnoopLogger::GetLogger().StopRecording();
      break;
    case HalState::kInit:
      VndSnoopLogger::GetLogger().StartNewRecording();
      break;
    case HalState::kFirmwareDownloading:
    case HalState::kFirmwareDownloadCompleted:
    case HalState::kFirmwareReady:
      break;
    case HalState::kBtChipReady:
      if (HalConfigLoader::GetLoader().IsAcceleratedBtOnSupported()) {
        if (old_state == HalState::kRunning) {
          VndSnoopLogger::GetLogger().StartNewRecording();
        } else if (old_state == HalState::kFirmwareReady) {
          if (HalConfigLoader::GetLoader().IsThreadDispatcherEnabled()) {
            LOG(INFO) << "Initialize Thread handler.";
            ThreadHandler::Initialize();
          }
        }
      }
      if (old_state == HalState::kFirmwareReady && hci_callback_ != nullptr) {
        // Once HAL changes to chip ready, it will automatically update to the
        // running state if the stack had called Initialize.
        defer_task = std::shared_ptr<void>(
            nullptr, [this](void*) { UpdateHalState(HalState::kRunning); });
      }
      break;
    case HalState::kRunning:
      VndSnoopLogger::GetLogger().StartNewRecording();
      if (HalConfigLoader::GetLoader().IsThreadDispatcherEnabled() &&
          !HalConfigLoader::GetLoader().IsAcceleratedBtOnSupported()) {
        LOG(INFO) << "Initialize Thread handler.";
        ThreadHandler::Initialize();
      }
      break;
    default:
      break;
  }

  if (hci_callback_ != nullptr) {
    hci_callback_->OnHalStateChanged(state, old_state);
  }
  HciRouterClientAgent::GetAgent().NotifyHalStateChange(state, old_state);
  TransportInterface::NotifyHalStateChange(state);
}

void HciRouterAsync::SendPacketToStack(const HalPacket& packet) {
  if (hci_callback_ != nullptr) {
    hci_callback_->OnPacketCallback(packet);
  }
}

void HciRouterAsync::OnTransportPacketReady(const HalPacket& packet) {
  ScopedWakelock wakelock(WakeSource::kRx);
  packet.SetDestination(PacketDestination::kHost);
  packet.SetSource(PacketSource::kController);

  if (hal_state_ == HalState::kShutdown) {
    LOG(WARNING) << __func__ << ": Hal is not ready to receive packets.";
    return;
  }

  VndSnoopLogger::GetLogger().Capture(packet,
                                      VndSnoopLogger::Direction::kIncoming);
  HandleReceivedPacket(packet);
}

bool HciRouterAsync::SendOrQueueCommand(
    const HalPacket& packet,
    const std::shared_ptr<HalPacketCallback> callback) {
  bool is_queue_busy = !hci_cmd_queue_.empty();

  hci_cmd_queue_.push({packet, callback});

  if (is_queue_busy) {
    HAL_LOG(DEBUG) << "command queued: " << packet.ToString();
    return true;
  }

  SetBusy(true);
  SendToTransport(packet);
  return true;
}

bool HciRouterAsync::SendToTransport(const HalPacket& packet) {
  ScopedWakelock wakelock(WakeSource::kTx);
  HAL_LOG(VERBOSE) << __func__ << ": " << packet.ToString();
  if (!TransportInterface::GetTransport().IsTransportActive()) {
    HAL_LOG(ERROR) << "Transport not active! packet: " << packet.ToString();
    return false;
  }
  VndSnoopLogger::GetLogger().Capture(packet,
                                      VndSnoopLogger::Direction::kOutgoing);

  return TransportInterface::GetTransport().Send(packet);
}

void HciRouterAsync::HandleReceivedPacket(const HalPacket& packet) {
  if (packet.IsCommandCompleteStatusEvent()) {
    HandleCommandCompleteOrCommandStatusEvent(packet);
    return;
  }
  if (HciRouterClientAgent::GetAgent().DispatchPacketToClients(packet) !=
          MonitorMode::kIntercept &&
      hci_callback_ != nullptr) {
    hci_callback_->OnPacketCallback(packet);
  }
}

void HciRouterAsync::HandleCommandCompleteOrCommandStatusEvent(
    const HalPacket& event) {
  auto state = HciRouterClientAgent::GetAgent().DispatchPacketToClients(event);
  switch (state) {
    case MonitorMode::kNone:
    case MonitorMode::kMonitor: {
      uint16_t opcode = event.GetCommandOpcodeFromGeneratedEvent();
      if (hci_cmd_queue_.empty() ||
          hci_cmd_queue_.front().command.GetCommandOpcode() != opcode) {
        LOG(ERROR)
            << "Unexpected command complete or command status event! opcode="
            << opcode;
        if (hci_callback_ != nullptr) {
          hci_callback_->OnPacketCallback(event);
        }
        return;
      }
      std::shared_ptr<HalPacketCallback> callback =
          hci_cmd_queue_.front().callback;

      if (callback == nullptr) {
        LOG(ERROR) << "Command callback is null!";
        if (hci_callback_ != nullptr) {
          hci_callback_->OnPacketCallback(event);
        }
      } else {
        (*callback)(event);
      }
      break;
    }
    case MonitorMode::kIntercept:
      break;
    case MonitorMode::kBypass:
      if (hci_callback_ != nullptr) {
        hci_callback_->OnPacketCallback(event);
      }
      return;
  }

  OnCommandCallbackCompleted();
}

void HciRouterAsync::OnCommandCallbackCompleted() {
  if (hci_cmd_queue_.empty()) {
    LOG(ERROR) << "Unexpected callback completed! "
               << "No command callback found in queue.";
    return;
  }
  hci_cmd_queue_.pop();

  bool has_queued_command = !hci_cmd_queue_.empty();
  SetBusy(has_queued_command);
  if (has_queued_command) {
    HalPacket queued_command = hci_cmd_queue_.front().command;
    SendToTransport(queued_command);
  }
}

bool HciRouterAsync::InitializeTransport() {
  HAL_LOG(INFO) << "Initializing Bluetooth transport.";
  if (transport_callback_ == nullptr) {
    HAL_LOG(ERROR) << "Bluetooth transport is null!";
    return false;
  }
  return TransportInterface::GetTransport().Initialize(transport_callback_);
}

bool HciRouterAsync::IsHalStateValid(HalState new_state) {
  return kHalStateMachine.at(hal_state_).count(new_state) > 0;
}

void HciRouterAsync::SetBusy(bool busy) {
  if (busy) {
    Wakelock::GetWakelock().Acquire(WakeSource::kHciBusy);
  } else {
    Wakelock::GetWakelock().Release(WakeSource::kHciBusy);
  }

  is_busy_ = busy;
  TransportInterface::GetTransport().SetHciRouterBusy(busy);
}

}  // namespace bluetooth_hal::hci
