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

#define LOG_TAG "bluetooth_hal.hci_router_interface"

#include "bluetooth_hal/hci_router_interface.h"

#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <utility>

#include "bluetooth_hal/config/hal_config_loader.h"
#include "bluetooth_hal/debug/debug_central.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/transport/transport_instance.h"

namespace bluetooth_hal::hci {

using ::bluetooth_hal::HalState;
using ::bluetooth_hal::config::HalConfigLoader;

void HciRouterInterface::OnTransportClosed() {
  LOG(INFO) << __func__ << ": Current transport is closed.";
}

void HciRouterInterface::OnTransportPacketReady(const HalPacket& packet) {
  HAL_LOG(VERBOSE) << __func__ << ": " << packet.ToString();
  DoInRouterThread(
      [this, packet]() { hci_router_async_->OnTransportPacketReady(packet); });
}

HciRouterInterface::HciRouterInterface()
    : HciRouterInterface(std::make_shared<HciRouterAsync>()) {}

HciRouterInterface::HciRouterInterface(
    std::shared_ptr<HciRouterAsync> hci_router_async)
    : hci_router_async_(hci_router_async) {
  // Try to initialize the router in the router thread if accelerated BT ON
  // feature is enabled.
  DoInRouterThread([this]() {
    if (HalConfigLoader::GetLoader().IsAcceleratedBtOnSupported()) {
      LOG(INFO) << "Powering ON Bluetooth chip for Accelerated BT ON.";
      hci_router_async_->InitializeModules(this);
    }
  });
}

bool HciRouterInterface::Initialize(
    const std::shared_ptr<HciRouterCallback>& callback) {
  return DoInRouterThread(
      [callback, this]() { hci_router_async_->Initialize(callback, this); });
}

void HciRouterInterface::Close() {
  // Close has to be synchronous to prevent initialize while closing.
  SynchronousDoInRouterThread([this]() { hci_router_async_->Close(); });
}

void HciRouterInterface::Cleanup() {
  // Cleanup has to be synchronous to prevent system shutdown before cleanup is
  // complete, which can cause potential power leakage in the hardware layer.
  SynchronousDoInRouterThread([this]() { hci_router_async_->Cleanup(); });
}

bool HciRouterInterface::Send(const HalPacket& packet) {
  return DoInRouterThread(
      [this, packet]() { hci_router_async_->Send(packet); });
}

bool HciRouterInterface::SendCommand(const HalPacket& packet,
                                     const HalPacketCallback& callback) {
  return DoInRouterThread([this, packet, callback]() {
    hci_router_async_->SendCommand(
        packet, std::make_shared<HalPacketCallback>(callback));
  });
}

bool HciRouterInterface::SendCommandNoAck(const HalPacket& packet) {
  return DoInRouterThread(
      [this, packet]() { hci_router_async_->SendCommandNoAck(packet); });
}

HalState HciRouterInterface::GetHalState() {
  return hci_router_async_->GetHalState();
}

void HciRouterInterface::UpdateHalState(HalState state) {
  DoInRouterThread(
      [this, state]() { hci_router_async_->UpdateHalState(state); });
}

void HciRouterInterface::SendPacketToStack(const HalPacket& packet) {
  DoInRouterThread(
      [this, packet]() { hci_router_async_->SendPacketToStack(packet); });
}

bool HciRouterInterface::SynchronousDoInRouterThread(
    std::function<void()> task) {
  return hci_router_async_->SynchronousDoInRouterThread(std::move(task));
}

bool HciRouterInterface::DoInRouterThread(std::function<void()> task) {
  return hci_router_async_->DoInRouterThread(std::move(task));
}

}  // namespace bluetooth_hal::hci
