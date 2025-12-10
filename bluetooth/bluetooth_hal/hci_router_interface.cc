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

#include "bluetooth_hal/config/hal_config_loader.h"
#include "bluetooth_hal/debug/debug_central.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/transport/transport_interface.h"

namespace bluetooth_hal::hci {

using ::bluetooth_hal::HalState;
using ::bluetooth_hal::config::HalConfigLoader;

void HciRouterInterface::OnTransportClosed() {
  LOG(INFO) << __func__ << ": Current transport is closed.";
}

void HciRouterInterface::OnTransportPacketReady(const HalPacket& packet) {
  HAL_LOG(VERBOSE) << __func__ << ": " << packet.ToString();
  hci_router_async_->DoInRouterThread(
      [this, packet]() { hci_router_async_->OnTransportPacketReady(packet); });
}

HciRouterInterface::HciRouterInterface()
    : HciRouterInterface(std::make_shared<HciRouterAsync>()) {}

HciRouterInterface::HciRouterInterface(
    std::shared_ptr<HciRouterAsync> hci_router_async)
    : hci_router_async_(hci_router_async) {
  // Try to initialize the router in the router thread if accelerated BT ON
  // feature is enabled.
  hci_router_async_->DoInRouterThread([this]() {
    if (HalConfigLoader::GetLoader().IsAcceleratedBtOnSupported()) {
      LOG(INFO) << "Powering ON Bluetooth chip for Accelerated BT ON.";
      hci_router_async_->InitializeModules(this);
    }
  });
}

bool HciRouterInterface::Initialize(
    const std::shared_ptr<HciRouterCallback>& callback) {
  return hci_router_async_->DoInRouterThread(
      [callback, this]() { hci_router_async_->Initialize(callback, this); });
}

void HciRouterInterface::Close() {
  std::promise<void> promise;
  auto future = promise.get_future();
  if (hci_router_async_->DoInRouterThread([this, &promise]() {
        hci_router_async_->Close();
        promise.set_value();
      })) {
    future.wait();
  }
}

void HciRouterInterface::Cleanup() {
  std::promise<void> promise;
  auto future = promise.get_future();
  if (hci_router_async_->DoInRouterThread([this, &promise]() {
        hci_router_async_->Cleanup();
        promise.set_value();
      })) {
    future.wait();
  }
}

bool HciRouterInterface::Send(const HalPacket& packet) {
  return hci_router_async_->DoInRouterThread(
      [this, packet]() { hci_router_async_->Send(packet); });
}

bool HciRouterInterface::SendCommand(const HalPacket& packet,
                                     const HalPacketCallback& callback) {
  return hci_router_async_->DoInRouterThread([this, packet, callback]() {
    hci_router_async_->SendCommand(
        packet, std::make_shared<HalPacketCallback>(callback));
  });
}

bool HciRouterInterface::SendCommandNoAck(const HalPacket& packet) {
  return hci_router_async_->DoInRouterThread(
      [this, packet]() { hci_router_async_->SendCommandNoAck(packet); });
}

HalState HciRouterInterface::GetHalState() {
  return hci_router_async_->GetHalState();
}

void HciRouterInterface::UpdateHalState(HalState state) {
  hci_router_async_->DoInRouterThread(
      [this, state]() { hci_router_async_->UpdateHalState(state); });
}

void HciRouterInterface::SendPacketToStack(const HalPacket& packet) {
  hci_router_async_->DoInRouterThread(
      [this, packet]() { hci_router_async_->SendPacketToStack(packet); });
}

}  // namespace bluetooth_hal::hci
