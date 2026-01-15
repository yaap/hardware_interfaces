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

#include "bluetooth_hal/hci_router_interface.h"

#include <cstdint>
#include <functional>
#include <memory>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hci_router_async.h"
#include "bluetooth_hal/hci_router_callback.h"
#include "bluetooth_hal/transport/transport_interface.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal::hci {
namespace {

using ::bluetooth_hal::HalState;
using ::bluetooth_hal::transport::TransportInterfaceCallback;
using ::testing::_;
using ::testing::An;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::Test;

class MockHciRouterCallback : public HciRouterCallback {
 public:
  MOCK_METHOD(void, OnCommandCallback, (const HalPacket& packet), (override));
  MOCK_METHOD(MonitorMode, OnPacketCallback, (const HalPacket& packet),
              (override));
  MOCK_METHOD(void, OnHalStateChanged,
              (const HalState new_state, const HalState old_state), (override));
};

class MockHciRouterAsync : public HciRouterAsync {
 public:
  MOCK_METHOD(bool, DoInRouterThread, (std::function<void()> task), (override));
  MOCK_METHOD(bool, SynchronousDoInRouterThread, (std::function<void()> task),
              (override));
  MOCK_METHOD(HalState, GetHalState, (), (override));
  MOCK_METHOD(void, Close, (), (override));
  MOCK_METHOD(void, Cleanup, (), (override));
  MOCK_METHOD(bool, Initialize,
              (const std::shared_ptr<HciRouterCallback>& callback,
               TransportInterfaceCallback* transport_callback),
              (override));
  MOCK_METHOD(bool, InitializeModules,
              (TransportInterfaceCallback * transport_callback), (override));
  MOCK_METHOD(bool, Send, (const HalPacket& packet), (override));
  MOCK_METHOD(bool, SendCommand,
              (const HalPacket& packet,
               const std::shared_ptr<HalPacketCallback>& callback),
              (override));
  MOCK_METHOD(bool, SendCommandNoAck, (const HalPacket& packet), (override));
  MOCK_METHOD(void, UpdateHalState, (HalState state), (override));
  MOCK_METHOD(void, SendPacketToStack, (const HalPacket& packet), (override));
  MOCK_METHOD(void, OnTransportPacketReady, (const HalPacket& packet),
              (override));
};

class HciRouterInterfaceTestInstance : public HciRouterInterface {
 public:
  HciRouterInterfaceTestInstance(
      std::shared_ptr<MockHciRouterAsync> mock_hci_router_async)
      : HciRouterInterface(mock_hci_router_async) {};
};

class HciRouterInterfaceTest : public Test {
 protected:
  HciRouterInterfaceTest() {}

  void SetUp() override {
    mock_hci_router_async_ = std::make_shared<MockHciRouterAsync>();
    mock_hci_router_callback_ = std::make_shared<MockHciRouterCallback>();
    // Expect DoInRouterThread to be called in the constructor for
    // AcceleratedBtOn.
    EXPECT_CALL(*mock_hci_router_async_,
                DoInRouterThread(An<std::function<void()>>()))
        .WillRepeatedly(Return(true));

    hci_router_interface_ = std::make_shared<HciRouterInterfaceTestInstance>(
        mock_hci_router_async_);
  }

  void TearDown() override {
    hci_router_interface_.reset();
    mock_hci_router_async_.reset();
    mock_hci_router_callback_.reset();
  }

  std::shared_ptr<HciRouterInterfaceTestInstance> hci_router_interface_;
  std::shared_ptr<MockHciRouterAsync> mock_hci_router_async_;
  std::shared_ptr<HciRouterCallback> mock_hci_router_callback_;
};

TEST_F(HciRouterInterfaceTest, Initialize) {
  std::function<void()> captured_task;
  EXPECT_CALL(*mock_hci_router_async_,
              DoInRouterThread(An<std::function<void()>>()))
      .WillOnce(DoAll(SaveArg<0>(&captured_task), Return(true)));
  EXPECT_CALL(*mock_hci_router_async_, Initialize(mock_hci_router_callback_, _))
      .Times(1);

  EXPECT_TRUE(hci_router_interface_->Initialize(mock_hci_router_callback_));
  captured_task();
}

TEST_F(HciRouterInterfaceTest, InitializeFailsWhenDoInRouterThreadFails) {
  EXPECT_CALL(*mock_hci_router_async_,
              DoInRouterThread(An<std::function<void()>>()))
      .WillOnce(Return(false));

  EXPECT_FALSE(hci_router_interface_->Initialize(mock_hci_router_callback_));
}

TEST_F(HciRouterInterfaceTest, Close) {
  EXPECT_CALL(*mock_hci_router_async_, SynchronousDoInRouterThread(_))
      .WillOnce(Invoke([](std::function<void()> task) {
        task();
        return true;
      }));
  EXPECT_CALL(*mock_hci_router_async_, Close()).Times(1);

  hci_router_interface_->Close();
}

TEST_F(HciRouterInterfaceTest, Cleanup) {
  EXPECT_CALL(*mock_hci_router_async_, SynchronousDoInRouterThread(_))
      .WillOnce(Invoke([](std::function<void()> task) {
        task();
        return true;
      }));
  EXPECT_CALL(*mock_hci_router_async_, Cleanup()).Times(1);

  hci_router_interface_->Cleanup();
}

TEST_F(HciRouterInterfaceTest, Send) {
  HalPacket packet({0x01, 0x02, 0x03});
  std::function<void()> captured_task;

  EXPECT_CALL(*mock_hci_router_async_,
              DoInRouterThread(An<std::function<void()>>()))
      .WillOnce(DoAll(SaveArg<0>(&captured_task), Return(true)));

  EXPECT_TRUE(hci_router_interface_->Send(packet));

  EXPECT_CALL(*mock_hci_router_async_, Send(packet)).Times(1);
  captured_task();
}

TEST_F(HciRouterInterfaceTest, SendCommand) {
  HalPacket packet({0x01, 0x02, 0x03});
  HalPacketCallback callback = [](const HalPacket&) {};
  std::function<void()> captured_task;

  EXPECT_CALL(*mock_hci_router_async_,
              DoInRouterThread(An<std::function<void()>>()))
      .WillOnce(DoAll(SaveArg<0>(&captured_task), Return(true)));

  EXPECT_TRUE(hci_router_interface_->SendCommand(packet, callback));

  EXPECT_CALL(*mock_hci_router_async_, SendCommand(packet, _)).Times(1);
  captured_task();
}

TEST_F(HciRouterInterfaceTest, SendCommandNoAck) {
  HalPacket packet({0x01, 0x02, 0x03});
  std::function<void()> captured_task;

  EXPECT_CALL(*mock_hci_router_async_,
              DoInRouterThread(An<std::function<void()>>()))
      .WillOnce(DoAll(SaveArg<0>(&captured_task), Return(true)));

  EXPECT_TRUE(hci_router_interface_->SendCommandNoAck(packet));

  EXPECT_CALL(*mock_hci_router_async_, SendCommandNoAck(packet)).Times(1);
  captured_task();
}

TEST_F(HciRouterInterfaceTest, GetHalState) {
  EXPECT_CALL(*mock_hci_router_async_, GetHalState())
      .WillOnce(Return(HalState::kShutdown));
  EXPECT_EQ(hci_router_interface_->GetHalState(), HalState::kShutdown);
}

TEST_F(HciRouterInterfaceTest, UpdateHalState) {
  HalState new_state = HalState::kRunning;
  std::function<void()> captured_task;
  EXPECT_CALL(*mock_hci_router_async_,
              DoInRouterThread(An<std::function<void()>>()))
      .WillOnce(DoAll(SaveArg<0>(&captured_task), Return(true)));

  hci_router_interface_->UpdateHalState(new_state);

  EXPECT_CALL(*mock_hci_router_async_, UpdateHalState(new_state)).Times(1);
  captured_task();
}

TEST_F(HciRouterInterfaceTest, SendPacketToStack) {
  HalPacket packet({0x01, 0x02, 0x03});
  std::function<void()> captured_task;

  EXPECT_CALL(*mock_hci_router_async_,
              DoInRouterThread(An<std::function<void()>>()))
      .WillOnce(DoAll(SaveArg<0>(&captured_task), Return(true)));

  hci_router_interface_->SendPacketToStack(packet);

  EXPECT_CALL(*mock_hci_router_async_, SendPacketToStack(packet)).Times(1);
  captured_task();
}

TEST_F(HciRouterInterfaceTest, OnTransportClosed) {
  // To make sure it doesn't crash.
  hci_router_interface_->OnTransportClosed();
}

TEST_F(HciRouterInterfaceTest, OnTransportPacketReady) {
  HalPacket packet({0x01, 0x02, 0x03});
  std::function<void()> captured_task;

  EXPECT_CALL(*mock_hci_router_async_,
              DoInRouterThread(An<std::function<void()>>()))
      .WillOnce(DoAll(SaveArg<0>(&captured_task), Return(true)));

  hci_router_interface_->OnTransportPacketReady(packet);

  EXPECT_CALL(*mock_hci_router_async_, OnTransportPacketReady(packet)).Times(1);
  captured_task();
}

}  // namespace
}  // namespace bluetooth_hal::hci
