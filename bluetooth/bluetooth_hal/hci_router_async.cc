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

#include "bluetooth_hal/hci_router_async.h"

namespace bluetooth_hal::hci {

using ::bluetooth_hal::transport::TransportInterfaceCallback;

HciRouterAsync::HciRouterAsync()
    : Worker(std::bind_front(&HciRouterAsync::TaskHandler, this)) {
  (void)hci_callback_;
  (void)transport_callback_;
  (void)hal_state_;
  (void)is_cleaning_up_;
  (void)hci_cmd_queue_;
  (void)is_busy_;
  (void)task_wakelock_mutex_;
  (void)wake_lock_votes_;
}

bool HciRouterAsync::DoInRouterThread(std::function<void()> /*task*/) {
  return false;
}

void HciRouterAsync::TaskHandler(RouterTask /*task*/) {}

void HciRouterAsync::VoteRouterTaskWakelock() {}

void HciRouterAsync::UnvoteRouterTaskWakelock() {}

bool HciRouterAsync::Initialize(
    const std::shared_ptr<HciRouterCallback>& /*callback*/,
    TransportInterfaceCallback* /*transport_callback*/) {
  return false;
}

bool HciRouterAsync::InitializeModules(
    TransportInterfaceCallback* /*transport_callback*/) {
  return false;
}

void HciRouterAsync::Close() {}

void HciRouterAsync::Cleanup() {}

bool HciRouterAsync::Send(const HalPacket& /*packet*/) { return false; }

bool HciRouterAsync::SendCommand(
    const HalPacket& /*packet*/,
    const std::shared_ptr<HalPacketCallback>& /*callback*/) {
  return false;
}

bool HciRouterAsync::SendCommandNoAck(const HalPacket& /*packet*/) {
  return false;
}

HalState HciRouterAsync::GetHalState() { return HalState::kShutdown; }

void HciRouterAsync::UpdateHalState(HalState /*state*/) {}

void HciRouterAsync::SendPacketToStack(const HalPacket& /*packet*/) {}

void HciRouterAsync::OnTransportPacketReady(const HalPacket& /*packet*/) {}

bool HciRouterAsync::SendToTransport(const HalPacket& /*packet*/) {
  return false;
}

void HciRouterAsync::HandleCommandCompleteOrCommandStatusEvent(
    const HalPacket& /*event*/) {}

bool HciRouterAsync::InitializeTransport() { return false; }

bool HciRouterAsync::IsHalStateValid(HalState /*new_state*/) { return false; }

void HciRouterAsync::HandleReceivedPacket(const HalPacket& /*packet*/) {}

bool HciRouterAsync::SendOrQueueCommand(
    const HalPacket& /*packet*/,
    const std::shared_ptr<HalPacketCallback> /*callback*/) {
  return false;
}

void HciRouterAsync::OnCommandCallbackCompleted() {}

void HciRouterAsync::SetBusy(bool /*busy*/) {}

}  // namespace bluetooth_hal::hci
