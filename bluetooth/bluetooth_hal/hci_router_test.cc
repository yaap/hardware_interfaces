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

#include "bluetooth_hal/hci_router.h"

#include <chrono>
#include <condition_variable>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_router_callback.h"
#include "bluetooth_hal/test/mock/mock_hal_config_loader.h"
#include "bluetooth_hal/test/mock/mock_hci_router_client_agent.h"
#include "bluetooth_hal/test/mock/mock_transport_interface.h"
#include "bluetooth_hal/test/mock/mock_vnd_snoop_logger.h"
#include "bluetooth_hal/test/mock/mock_wakelock.h"
#include "bluetooth_hal/transport/transport_interface.h"
#include "com_android_bluetooth_bluetooth_hal_flags.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal::hci {
namespace {

using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::Test;

using ::bluetooth_hal::HalState;
using ::bluetooth_hal::config::MockHalConfigLoader;
using ::bluetooth_hal::debug::MockVndSnoopLogger;
using ::bluetooth_hal::transport::MockTransportInterface;
using ::bluetooth_hal::transport::TransportInterfaceCallback;
using ::bluetooth_hal::util::power::MockWakelock;

HalPacketCallback EmptyHalPacketCallback =
    []([[maybe_unused]] const HalPacket& packet) {};

class FakeHciRouterCallback : public HciRouterCallback {
 public:
  FakeHciRouterCallback() = default;
  void OnCommandCallback(const HalPacket& packet) override {
    OnPacketCallback(packet);
  };
  MOCK_METHOD(MonitorMode, OnPacketCallback, (const HalPacket& packet),
              (override));
  MOCK_METHOD(void, OnHalStateChanged,
              (const HalState new_state, const HalState old_state), (override));
};

class HciRouterTest : public Test {
 protected:
  static void SetUpTestSuite() {}

  void SetUp() override {
    MockHciRouterClientAgent::SetMockAgent(&mock_hci_router_client_agent_);
    fake_hci_callback_ = std::make_shared<FakeHciRouterCallback>();

    ON_CALL(mock_transport_interface_, IsTransportActive())
        .WillByDefault(Return(true));
    ON_CALL(mock_transport_interface_, Send(_))
        .WillByDefault(Invoke(this, &HciRouterTest::OnSendToTransport));
    ON_CALL(mock_transport_interface_, Initialize(_))
        .WillByDefault(Invoke(
            [this](TransportInterfaceCallback* transport_interface_callback) {
              this->transport_interface_callback_ =
                  transport_interface_callback;
              return true;
            }));
    ON_CALL(mock_transport_interface_, SetHciRouterBusy(_))
        .WillByDefault(
            Invoke(this, &HciRouterTest::OnSetHciRouterBusyInTransport));
    ON_CALL(mock_hal_config_loader_, IsAcceleratedBtOnSupported())
        .WillByDefault(Return(false));
    ON_CALL(*fake_hci_callback_, OnHalStateChanged(_, _))
        .WillByDefault(DoAll(SaveArg<0>(&new_state_), SaveArg<1>(&old_state_)));
    ON_CALL(*fake_hci_callback_, OnPacketCallback(_))
        .WillByDefault(
            DoAll(SaveArg<0>(&hal_packet_), Return(MonitorMode::kNone)));

    MockTransportInterface::SetMockTransport(&mock_transport_interface_);
    MockHalConfigLoader::SetMockLoader(&mock_hal_config_loader_);
    MockWakelock::SetMockWakelock(&mock_wakelock_);
    MockVndSnoopLogger::SetMockVndSnoopLogger(&mock_vnd_snoop_logger_);

    router_ = &HciRouter::GetRouter();
    InitializeHciRouter();
  }

  void TearDown() override {
    CleanupHciRouter();
    command_sent_promises_.clear();
    command_sent_futures_.clear();
  }

  void InitializeHciRouter() {
    EXPECT_CALL(mock_transport_interface_, Initialize(_)).Times(1);
    router_->Initialize(fake_hci_callback_);
    CompleteFirmwareDownloadAndStackInit();
  }

  void CleanupHciRouter() {
    EXPECT_CALL(mock_transport_interface_, CleanupTransport())
        .Times(AtLeast(1));
    router_->Cleanup();
    ASSERT_EQ(new_state_, HalState::kShutdown);
    ASSERT_EQ(router_->GetHalState(), HalState::kShutdown);
  }

  void OnSetHciRouterBusyInTransport(bool busy) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      is_router_busy_ = busy;
    }
    cv_.notify_one();
  }

  /**
   * @brief Get the HCI router busy state, waiting up to 100ms for updates.
   *
   * As HCI router busy state could be changed asynchronously, we wait a bit for
   * `is_router_busy_` to change (notified by `OnSetHciRouterBusyInTransport`).
   * Timeout may indicate the value was not set or has set earlier.
   *
   * @return The current or last known value of `is_router_busy_`.
   *
   */
  bool GetIsRouterBusy() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait_for(lock, std::chrono::milliseconds(10));
    return is_router_busy_;
  }

  void CompleteFirmwareDownloadAndStackInit() {
    // Mock the chip provisioner firmware download behaivor.
    router_->UpdateHalState(HalState::kPreFirmwareDownload);
    router_->UpdateHalState(HalState::kFirmwareDownloading);
    router_->UpdateHalState(HalState::kFirmwareDownloadCompleted);
    router_->UpdateHalState(HalState::kFirmwareReady);

    Mock::VerifyAndClearExpectations(&(*fake_hci_callback_));

    // Check state is Running.
    std::vector<HalState> state_changes;
    EXPECT_CALL(*fake_hci_callback_, OnHalStateChanged(_, _))
        .Times(2)
        .WillRepeatedly(Invoke(
            [&](HalState new_state, [[maybe_unused]] HalState old_state) {
              state_changes.push_back(new_state);
            }));

    router_->UpdateHalState(HalState::kBtChipReady);

    EXPECT_EQ(state_changes.size(), 2);
    EXPECT_EQ(state_changes[0], HalState::kBtChipReady);
    EXPECT_EQ(state_changes[1], HalState::kRunning);

    // Without accelerated BT enabled, once HAL changes to `kBtChipReady`, it
    // will automatically update to the `kRunning`.
    EXPECT_EQ(router_->GetHalState(), HalState::kRunning);

    Mock::VerifyAndClearExpectations(&(*fake_hci_callback_));
  }

  void CompleteResetFirmwareWithAcceleratedBtOn() {
    // Mock the chip provisioner reset behavior.
    if (router_->GetHalState() != HalState::kBtChipReady &&
        router_->GetHalState() != HalState::kRunning) {
      return;
    }

    HalState target_state = router_->GetHalState() == HalState::kBtChipReady
                                ? HalState::kRunning
                                : HalState::kBtChipReady;
    router_->UpdateHalState(target_state);

    ASSERT_EQ(new_state_, target_state);
    ASSERT_EQ(router_->GetHalState(), target_state);
  }

  void MarkPacketAsSent(const HalPacket& packet) {
    auto it = command_sent_promises_.find(packet);
    if (it != command_sent_promises_.end()) {
      it->second.set_value();
    }
  }

  bool OnSendToTransport(const HalPacket& packet) {
    // To let the on_packet_ready know the command is sent.
    MarkPacketAsSent(packet);
    return true;
  }

  void WaitPacketSentToTransport(const HalPacket& packet) {
    bool success = false;
    for (int i = 0; i < 3; i++) {
      auto it = command_sent_futures_.find(packet);
      if (it != command_sent_futures_.end()) {
        auto status = it->second.wait_for(std::chrono::seconds(3));
        // Expect the packet was sent to the transport in time.
        EXPECT_EQ(status, std::future_status::ready);
        success = true;
        break;
      }
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    EXPECT_TRUE(success);
  }

  /**
   * @brief Send packet to the HciRouter, also prepare a future lock for
   * WaitPacketSentToTransport(const HalPacket& packet).
   *
   * As HciRouter::Send* is handled asynchronously, a future lock is required to
   * determine weither the packet has been sent to the transport layer.
   *
   * @return true if the packet is posted to the asynchronous thread, otherwise
   * false.
   */
  bool SendToRouter(const HalPacket& packet) {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    command_sent_promises_[packet] = std::move(promise);
    command_sent_futures_[packet] = std::move(future);
    return router_->Send(packet);
  }

  bool SendCommandToRouter(const HalPacket& packet,
                           const HalPacketCallback& callback) {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    command_sent_promises_[packet] = std::move(promise);
    command_sent_futures_[packet] = std::move(future);
    return router_->SendCommand(packet, callback);
  }

  bool SendCommandNoAckToRouter(const HalPacket& packet) {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    command_sent_promises_[packet] = std::move(promise);
    command_sent_futures_[packet] = std::move(future);
    return router_->SendCommandNoAck(packet);
  }

  void ExpectHalStateChange(HalState new_state, HalState old_state,
                            int count = 1) {
    EXPECT_CALL(*fake_hci_callback_, OnHalStateChanged(new_state, old_state))
        .Times(count);
    EXPECT_CALL(mock_hci_router_client_agent_,
                NotifyHalStateChange(new_state, old_state))
        .Times(count);
    EXPECT_CALL(mock_transport_interface_, NotifyHalStateChange(new_state))
        .Times(count);
  }

  FakeHciRouterCallback fake_router_callback_;
  std::shared_ptr<FakeHciRouterCallback> fake_hci_callback_;
  HciRouter* router_;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool is_router_busy_ = false;
  HalState new_state_;
  HalState old_state_;
  HalPacket hal_packet_;
  TransportInterfaceCallback* transport_interface_callback_;
  MockTransportInterface mock_transport_interface_;
  MockHalConfigLoader mock_hal_config_loader_;
  MockWakelock mock_wakelock_;
  MockVndSnoopLogger mock_vnd_snoop_logger_;
  MockHciRouterClientAgent mock_hci_router_client_agent_;
  std::map<HalPacket, std::promise<void>> command_sent_promises_;
  std::map<HalPacket, std::future<void>> command_sent_futures_;
};

TEST_F(HciRouterTest, InitializeWithAcceleratedBtOn) {
  // Power up the Bluetooth chip.
  ON_CALL(mock_hal_config_loader_, IsAcceleratedBtOnSupported())
      .WillByDefault(Return(true));

  // Turn off Bluetooth, but without cleanup the transport layer.
  router_->Close();
  CompleteResetFirmwareWithAcceleratedBtOn();

  // Turn on Bluetooth from kBtChipReady state, skip firmware download.
  router_->Initialize(fake_hci_callback_);
  CompleteResetFirmwareWithAcceleratedBtOn();

  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  cmd_reset.SetSource(PacketSource::kStack);
  HalPacket evt_reset({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(1);
  EXPECT_TRUE(SendToRouter(cmd_reset));
  WaitPacketSentToTransport(cmd_reset);
  EXPECT_TRUE(GetIsRouterBusy());
  transport_interface_callback_->OnTransportPacketReady(evt_reset);
  EXPECT_FALSE(GetIsRouterBusy());
  EXPECT_EQ(hal_packet_, evt_reset);

  // Disable Accelerated BT ON for test tear down.
  ON_CALL(mock_hal_config_loader_, IsAcceleratedBtOnSupported())
      .WillByDefault(Return(false));
}

TEST_F(HciRouterTest, HandleSendAclData) {
  HalPacket acl_data({0x02, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  acl_data.SetSource(PacketSource::kStack);
  EXPECT_CALL(mock_transport_interface_, Send(acl_data)).Times(1);
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(acl_data))
      .WillOnce(Invoke([&](const HalPacket& captured_packet) {
        EXPECT_EQ(captured_packet.GetDestination(),
                  PacketDestination::kController);
        EXPECT_EQ(captured_packet.GetSource(), PacketSource::kStack);
        return MonitorMode::kNone;
      }));

  EXPECT_TRUE(SendToRouter(acl_data));
  WaitPacketSentToTransport(acl_data);
  EXPECT_FALSE(GetIsRouterBusy());
}

TEST_F(HciRouterTest, HandleSendHciCommand) {
  HalPacket cmd({0x01, 0x03, 0x0c, 0x00});
  cmd.SetSource(PacketSource::kStack);
  EXPECT_CALL(mock_transport_interface_, Send(cmd)).Times(1);
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(cmd))
      .WillOnce(Invoke([&](const HalPacket& captured_packet) {
        EXPECT_EQ(captured_packet.GetDestination(),
                  PacketDestination::kController);
        EXPECT_EQ(captured_packet.GetSource(), PacketSource::kStack);
        return MonitorMode::kNone;
      }));

  EXPECT_TRUE(SendToRouter(cmd));
  WaitPacketSentToTransport(cmd);
  EXPECT_TRUE(GetIsRouterBusy());
}

TEST_F(HciRouterTest, HandleSendHciCommandTwiceWithoutEventWithoutFlag) {
  // TODO: b/439994729 - remove this test when deprecating the flag.
  set_com_android_bluetooth_bluetooth_hal_flags_handle_recursive_packets_from_router_clients(
      false);

  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  cmd_reset.SetSource(PacketSource::kStack);
  HalPacket cmd_set_host_le_support({0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00});
  cmd_set_host_le_support.SetSource(PacketSource::kStack);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(0);
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(cmd_reset))
      .WillOnce(Invoke([&](const HalPacket& captured_packet) {
        EXPECT_EQ(captured_packet.GetDestination(),
                  PacketDestination::kController);
        return MonitorMode::kNone;
      }));
  EXPECT_CALL(mock_hci_router_client_agent_,
              DispatchPacketToClients(cmd_set_host_le_support))
      .Times(0);

  EXPECT_TRUE(SendToRouter(cmd_reset));
  WaitPacketSentToTransport(cmd_reset);
  EXPECT_TRUE(GetIsRouterBusy());

  EXPECT_TRUE(SendToRouter(cmd_set_host_le_support));
  // The packet should not reach to the router, so no need to wait here.
  EXPECT_TRUE(GetIsRouterBusy());
}

TEST_F(HciRouterTest, HandleSendHciCommandTwiceWithoutEvent) {
  set_com_android_bluetooth_bluetooth_hal_flags_handle_recursive_packets_from_router_clients(
      true);

  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  cmd_reset.SetSource(PacketSource::kStack);
  HalPacket cmd_set_host_le_support({0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00});
  cmd_set_host_le_support.SetSource(PacketSource::kStack);

  // The second command is stays in the HCI queue until the first command is
  // completed, however it will be passed to router client agent first for
  // potential packet interceptions.
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(cmd_reset))
      .WillOnce(Invoke([&](const HalPacket& captured_packet) {
        EXPECT_EQ(captured_packet.GetSource(), PacketSource::kStack);
        EXPECT_EQ(captured_packet.GetDestination(),
                  PacketDestination::kController);
        return MonitorMode::kNone;
      }));
  EXPECT_CALL(mock_hci_router_client_agent_,
              DispatchPacketToClients(cmd_set_host_le_support))
      .WillOnce(Invoke([&](const HalPacket& captured_packet) {
        EXPECT_EQ(captured_packet.GetSource(), PacketSource::kStack);
        EXPECT_EQ(captured_packet.GetDestination(),
                  PacketDestination::kController);
        return MonitorMode::kNone;
      }));
  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(0);

  EXPECT_TRUE(SendToRouter(cmd_reset));
  WaitPacketSentToTransport(cmd_reset);
  EXPECT_TRUE(GetIsRouterBusy());

  EXPECT_TRUE(SendToRouter(cmd_set_host_le_support));
  // The packet should not reach to the router, so no need to wait here.
  EXPECT_TRUE(GetIsRouterBusy());
}

TEST_F(HciRouterTest, HandleSendHciCommandTwiceWithEvent) {
  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  cmd_reset.SetSource(PacketSource::kStack);
  HalPacket evt_reset({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  HalPacket cmd_set_host_le_support({0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00});
  cmd_set_host_le_support.SetSource(PacketSource::kStack);

  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(evt_reset)).Times(1);
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(cmd_reset))
      .WillOnce(Invoke([&](const HalPacket& captured_packet) {
        EXPECT_EQ(captured_packet.GetDestination(),
                  PacketDestination::kController);
        return MonitorMode::kNone;
      }));
  EXPECT_CALL(mock_hci_router_client_agent_,
              DispatchPacketToClients(cmd_set_host_le_support))
      .WillOnce(Invoke([&](const HalPacket& captured_packet) {
        EXPECT_EQ(captured_packet.GetDestination(),
                  PacketDestination::kController);
        return MonitorMode::kNone;
      }));
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(evt_reset))
      .WillOnce(Invoke([&](const HalPacket& captured_packet) {
        EXPECT_EQ(captured_packet.GetDestination(), PacketDestination::kHost);
        return MonitorMode::kNone;
      }));

  // Send the first command.
  EXPECT_TRUE(SendToRouter(cmd_reset));
  WaitPacketSentToTransport(cmd_reset);
  EXPECT_TRUE(GetIsRouterBusy());
  // Receive the event for the first command, and pass to the stack callback.
  transport_interface_callback_->OnTransportPacketReady(evt_reset);
  EXPECT_FALSE(GetIsRouterBusy());
  EXPECT_EQ(hal_packet_, evt_reset);
  // Send the second command.
  EXPECT_TRUE(SendToRouter(cmd_set_host_le_support));
  WaitPacketSentToTransport(cmd_set_host_le_support);
  EXPECT_TRUE(GetIsRouterBusy());
}

TEST_F(HciRouterTest, HandleSendHciCommandTwiceWithLateEvent) {
  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  cmd_reset.SetSource(PacketSource::kStack);
  HalPacket evt_reset({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  HalPacket cmd_set_host_le_support({0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00});
  cmd_set_host_le_support.SetSource(PacketSource::kStack);
  HalPacket evt_set_host_le_support({0x04, 0x0e, 0x04, 0x01, 0x6d, 0x0c, 0x00});

  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(2);

  // Send the first command.
  EXPECT_TRUE(SendToRouter(cmd_reset));
  WaitPacketSentToTransport(cmd_reset);
  EXPECT_TRUE(GetIsRouterBusy());
  // Send the second command.
  EXPECT_TRUE(SendToRouter(cmd_set_host_le_support));
  EXPECT_TRUE(GetIsRouterBusy());
  // Receive the event for the first command, and pass to the stack callback.
  transport_interface_callback_->OnTransportPacketReady(evt_reset);
  EXPECT_EQ(hal_packet_, evt_reset);
  // Router stays busy as there is still a command in the queue.
  EXPECT_TRUE(GetIsRouterBusy());
  // Wait for the enqueued command to be sent to the transport
  WaitPacketSentToTransport(cmd_set_host_le_support);
  // Receive the event for the second command.
  transport_interface_callback_->OnTransportPacketReady(
      evt_set_host_le_support);
  EXPECT_EQ(hal_packet_, evt_set_host_le_support);
  EXPECT_FALSE(GetIsRouterBusy());
}

TEST_F(HciRouterTest, HandleSendCommandTwiceWithoutEvent) {
  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  cmd_reset.SetSource(PacketSource::kStack);
  HalPacket cmd_set_host_le_support({0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00});
  cmd_set_host_le_support.SetSource(PacketSource::kStack);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(0);

  EXPECT_TRUE(SendCommandToRouter(cmd_reset, EmptyHalPacketCallback));
  WaitPacketSentToTransport(cmd_reset);
  EXPECT_TRUE(GetIsRouterBusy());
  EXPECT_TRUE(
      SendCommandToRouter(cmd_set_host_le_support, EmptyHalPacketCallback));
  EXPECT_TRUE(GetIsRouterBusy());
}

TEST_F(HciRouterTest, HandleSendCommandTwiceWithEvent) {
  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  cmd_reset.SetSource(PacketSource::kStack);
  HalPacket evt_reset({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  HalPacket cmd_set_host_le_support({0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00});
  cmd_set_host_le_support.SetSource(PacketSource::kStack);

  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(0);

  // Send the first command with a client callback.
  HalPacket event;
  EXPECT_TRUE(SendCommandToRouter(
      cmd_reset, [&event](const HalPacket& packet) { event = packet; }));
  WaitPacketSentToTransport(cmd_reset);
  EXPECT_TRUE(GetIsRouterBusy());
  // Receive the event for the first command, check if the event is sent to
  // the client callback.
  transport_interface_callback_->OnTransportPacketReady(evt_reset);
  EXPECT_FALSE(GetIsRouterBusy());
  EXPECT_EQ(event, evt_reset);
  // Send the second command.
  EXPECT_TRUE(
      SendCommandToRouter(cmd_set_host_le_support, EmptyHalPacketCallback));
  WaitPacketSentToTransport(cmd_set_host_le_support);
  EXPECT_TRUE(GetIsRouterBusy());
}

TEST_F(HciRouterTest, HandleSendCommandTwiceWithLateEvent) {
  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  cmd_reset.SetSource(PacketSource::kStack);
  HalPacket evt_reset({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  HalPacket cmd_set_host_le_support({0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00});
  cmd_set_host_le_support.SetSource(PacketSource::kStack);
  HalPacket evt_set_host_le_support({0x04, 0x0e, 0x04, 0x01, 0x6d, 0x0c, 0x00});

  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(0);

  // Send the first command with a client callback.
  HalPacket event;
  EXPECT_TRUE(SendCommandToRouter(
      cmd_reset, [&event](const HalPacket& packet) { event = packet; }));
  WaitPacketSentToTransport(cmd_reset);
  EXPECT_TRUE(GetIsRouterBusy());
  // Send the second command.
  EXPECT_TRUE(
      SendCommandToRouter(cmd_set_host_le_support, EmptyHalPacketCallback));
  EXPECT_TRUE(GetIsRouterBusy());
  // Receive the event for the first command, check if the event is sent to
  // the client callback.
  transport_interface_callback_->OnTransportPacketReady(evt_reset);
  EXPECT_TRUE(GetIsRouterBusy());
  EXPECT_EQ(event, evt_reset);

  // Wait for the enqueued command to be sent to the transport
  WaitPacketSentToTransport(cmd_set_host_le_support);
  // Receive the event for the second command.
  transport_interface_callback_->OnTransportPacketReady(
      evt_set_host_le_support);
  EXPECT_FALSE(GetIsRouterBusy());
}

TEST_F(HciRouterTest, HandleSendHciCommandInCallback) {
  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  cmd_reset.SetSource(PacketSource::kStack);
  HalPacket evt_reset({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  HalPacket cmd_set_host_le_support({0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00});
  cmd_set_host_le_support.SetSource(PacketSource::kStack);
  HalPacket evt_set_host_le_support({0x04, 0x0e, 0x04, 0x01, 0x6d, 0x0c, 0x00});

  // Expect both cmd_reset and cmd_set_host_le_support are sent to the
  // transport layer, and no callback to the stack.
  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(0);

  // Send the first command with a client callback, and send the second
  // command in the client callback.
  HalPacket event;
  EXPECT_TRUE(SendCommandToRouter(cmd_reset, [&event, &cmd_set_host_le_support,
                                              this](const HalPacket& packet) {
    event = packet;
    EXPECT_TRUE(
        SendCommandToRouter(cmd_set_host_le_support, EmptyHalPacketCallback));
  }));
  WaitPacketSentToTransport(cmd_reset);
  EXPECT_TRUE(GetIsRouterBusy());

  // Receive the generated event for the first command, check if the second
  // command is properly sent.
  transport_interface_callback_->OnTransportPacketReady(evt_reset);
  EXPECT_EQ(event, evt_reset);
  EXPECT_TRUE(GetIsRouterBusy());
  // Check the second command is properly handled.
  WaitPacketSentToTransport(cmd_set_host_le_support);
  transport_interface_callback_->OnTransportPacketReady(
      evt_set_host_le_support);
  EXPECT_FALSE(GetIsRouterBusy());
}

TEST_F(HciRouterTest, HandleSendHciCommandInCallbackAfterAnotherSendCommand) {
  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  cmd_reset.SetSource(PacketSource::kStack);
  HalPacket evt_reset({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  HalPacket cmd_set_min_enc_key_size({0x01, 0x84, 0x0c, 0x01, 0x07});
  cmd_set_min_enc_key_size.SetSource(PacketSource::kStack);
  HalPacket evt_set_min_enc_key_size(
      {0x04, 0x0e, 0x04, 0x01, 0x84, 0x0c, 0x00});
  HalPacket cmd_set_host_le_support({0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00});
  cmd_set_host_le_support.SetSource(PacketSource::kStack);
  HalPacket evt_set_host_le_support({0x04, 0x0e, 0x04, 0x01, 0x6d, 0x0c, 0x00});

  // Expect both cmd_reset and cmd_set_host_le_support are sent to the
  // transport layer, and no callback to the stack.
  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_min_enc_key_size))
      .Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(0);

  // Send the first command with a client callback, and send the second
  // command in the client callback.
  HalPacket event;
  EXPECT_TRUE(SendCommandToRouter(cmd_reset, [&event, &cmd_set_host_le_support,
                                              this](const HalPacket& packet) {
    event = packet;
    EXPECT_TRUE(
        SendCommandToRouter(cmd_set_host_le_support, EmptyHalPacketCallback));
  }));
  WaitPacketSentToTransport(cmd_reset);
  EXPECT_TRUE(GetIsRouterBusy());
  EXPECT_TRUE(SendCommandToRouter(
      cmd_set_min_enc_key_size,
      [&event](const HalPacket& packet) { event = packet; }));
  EXPECT_TRUE(GetIsRouterBusy());

  // Receive three generated events in order
  transport_interface_callback_->OnTransportPacketReady(evt_reset);
  EXPECT_EQ(event, evt_reset);
  EXPECT_TRUE(GetIsRouterBusy());

  WaitPacketSentToTransport(cmd_set_min_enc_key_size);
  transport_interface_callback_->OnTransportPacketReady(
      evt_set_min_enc_key_size);
  EXPECT_EQ(event, evt_set_min_enc_key_size);
  EXPECT_TRUE(GetIsRouterBusy());

  WaitPacketSentToTransport(cmd_set_host_le_support);
  transport_interface_callback_->OnTransportPacketReady(
      evt_set_host_le_support);
  EXPECT_FALSE(GetIsRouterBusy());
}

TEST_F(HciRouterTest, HandleSendDebugInfoCommandAfterSendCommand) {
  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  cmd_reset.SetSource(PacketSource::kStack);
  HalPacket cmd_debug_info({0x01, 0x5B, 0xFD, 0x00});
  cmd_debug_info.SetSource(PacketSource::kStack);

  // Expect both cmd_reset and cmd_debug_info are sent to the transport layer.
  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_debug_info)).Times(1);

  // Send the first command with a client callback.
  HalPacket event;
  EXPECT_TRUE(SendCommandToRouter(cmd_reset, EmptyHalPacketCallback));
  WaitPacketSentToTransport(cmd_reset);
  EXPECT_TRUE(GetIsRouterBusy());

  // Send the second command (Debug Info) without waiting the event for the
  // first command.
  EXPECT_TRUE(SendCommandToRouter(cmd_debug_info, EmptyHalPacketCallback));
  WaitPacketSentToTransport(cmd_debug_info);
}

TEST_F(HciRouterTest, HandleSendCommandNoAck) {
  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  cmd_reset.SetSource(PacketSource::kStack);
  HalPacket cmd_set_host_le_support({0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00});
  cmd_set_host_le_support.SetSource(PacketSource::kStack);

  // Check if the received event is dispatched to client agent and transport.
  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(1);
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(cmd_reset))
      .WillOnce(Invoke([&](const HalPacket& captured_packet) {
        EXPECT_EQ(captured_packet.GetDestination(),
                  PacketDestination::kController);
        return MonitorMode::kNone;
      }));
  EXPECT_CALL(mock_hci_router_client_agent_,
              DispatchPacketToClients(cmd_set_host_le_support))
      .WillOnce(Invoke([&](const HalPacket& captured_packet) {
        EXPECT_EQ(captured_packet.GetDestination(),
                  PacketDestination::kController);
        return MonitorMode::kNone;
      }));

  // Send the first command.
  EXPECT_TRUE(SendCommandNoAckToRouter(cmd_reset));
  WaitPacketSentToTransport(cmd_reset);
  EXPECT_FALSE(GetIsRouterBusy());
  // Send the second command.
  EXPECT_TRUE(SendCommandNoAckToRouter(cmd_set_host_le_support));
  WaitPacketSentToTransport(cmd_set_host_le_support);
  EXPECT_FALSE(GetIsRouterBusy());
}

TEST_F(HciRouterTest, HandleDispatchPacketToClientsMonitorNone) {
  HalPacket event({0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});

  // Check if the received event is dispatched to client agent and stack.
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(event))
      .WillOnce(Invoke([&](const HalPacket& captured_event) {
        EXPECT_EQ(captured_event.GetDestination(), PacketDestination::kHost);
        return MonitorMode::kNone;
      }));
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(event)).Times(1);

  transport_interface_callback_->OnTransportPacketReady(event);
  EXPECT_EQ(hal_packet_, event);
}

TEST_F(HciRouterTest, HandleDispatchPacketToClientsMonitorMonitor) {
  HalPacket event({0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});

  // Check if the received event is dispatched to client agent and stack.
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(event))
      .WillOnce(Invoke([&](const HalPacket& captured_event) {
        EXPECT_EQ(captured_event.GetDestination(), PacketDestination::kHost);
        return MonitorMode::kMonitor;
      }));
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(event)).Times(1);

  transport_interface_callback_->OnTransportPacketReady(event);
  EXPECT_EQ(hal_packet_, event);
}

TEST_F(HciRouterTest, HandleDispatchPacketToClientsMonitorIntercept) {
  HalPacket event({0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});

  // Check if the received event is only dispatched to the agent.
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(event))
      .WillOnce(Invoke([&](const HalPacket& captured_event) {
        EXPECT_EQ(captured_event.GetDestination(), PacketDestination::kHost);
        EXPECT_EQ(captured_event.GetSource(), PacketSource::kController);
        return MonitorMode::kIntercept;
      }));
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(0);

  transport_interface_callback_->OnTransportPacketReady(event);
}

TEST_F(HciRouterTest, HandleOnAclDataCallback) {
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(1);
  HalPacket packet({0x02, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  transport_interface_callback_->OnTransportPacketReady(packet);
  EXPECT_EQ(hal_packet_, packet);
  EXPECT_EQ(hal_packet_.GetSource(), PacketSource::kController);
}

TEST_F(HciRouterTest, HandleOnScoDataCallback) {
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(1);
  HalPacket packet({0x03, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  transport_interface_callback_->OnTransportPacketReady(packet);
  EXPECT_EQ(hal_packet_, packet);
  EXPECT_EQ(hal_packet_.GetSource(), PacketSource::kController);
}

TEST_F(HciRouterTest, HandleOnIsoDataCallback) {
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(1);
  HalPacket packet({0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  transport_interface_callback_->OnTransportPacketReady(packet);
  EXPECT_EQ(hal_packet_, packet);
  EXPECT_EQ(hal_packet_.GetSource(), PacketSource::kController);
}

TEST_F(HciRouterTest, HandleDispatchPacketToClientsInterceptThreadData) {
  FakeHciRouterCallback fake_router_callback;
  HalPacket thread_data({0x70, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});

  ON_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(_))
      .WillByDefault(Return(MonitorMode::kIntercept));

  // Expect router callback is called, but stack callback is not.
  EXPECT_CALL(mock_hci_router_client_agent_,
              DispatchPacketToClients(thread_data))
      .Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(0);

  transport_interface_callback_->OnTransportPacketReady(thread_data);
}

TEST_F(HciRouterTest, HandleSendPacketToStack) {
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(1);
  HalPacket packet({0x02, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  router_->SendPacketToStack(packet);
  EXPECT_EQ(hal_packet_, packet);
}

TEST_F(HciRouterTest, HandleUpdateHalState) {
  // A second shutdown is called in the test Cleanup.
  ExpectHalStateChange(HalState::kShutdown, HalState::kRunning, 2);
  ExpectHalStateChange(HalState::kInit, HalState::kShutdown);
  ExpectHalStateChange(HalState::kPreFirmwareDownload, HalState::kInit);
  ExpectHalStateChange(HalState::kFirmwareDownloading,
                       HalState::kPreFirmwareDownload);
  ExpectHalStateChange(HalState::kFirmwareDownloadCompleted,
                       HalState::kFirmwareDownloading);
  ExpectHalStateChange(HalState::kFirmwareReady,
                       HalState::kFirmwareDownloadCompleted);
  ExpectHalStateChange(HalState::kBtChipReady, HalState::kFirmwareReady);
  ExpectHalStateChange(HalState::kRunning, HalState::kBtChipReady);
  router_->UpdateHalState(HalState::kShutdown);
  router_->UpdateHalState(HalState::kInit);
  router_->UpdateHalState(HalState::kPreFirmwareDownload);
  router_->UpdateHalState(HalState::kFirmwareDownloading);
  router_->UpdateHalState(HalState::kFirmwareDownloadCompleted);
  router_->UpdateHalState(HalState::kFirmwareReady);
  // Without accelerated BT enabled, once HAL changes to `kBtChipReady`, it
  // will automatically update to the `kRunning`.
  router_->UpdateHalState(HalState::kBtChipReady);
}

TEST_F(HciRouterTest, HandleCleanupAndRxAtTheSameTime) {
  std::mutex m;
  std::condition_variable cv;
  bool rx_dispatched = false;

  // Override CleanupTransport(), force it wait for the next RX to be completed.
  ON_CALL(mock_transport_interface_, CleanupTransport())
      .WillByDefault(Invoke([&m, &cv, &rx_dispatched]() {
        std::unique_lock<std::mutex> lock(m);
        bool result = cv.wait_for(lock, std::chrono::seconds(3),
                                  [&] { return rx_dispatched; });
        EXPECT_TRUE(rx_dispatched)
            << "Timeout: Main thread never signaled rx_dispatched";
      }));

  // Start cleaning up the router.
  auto cleanup_thread = std::thread([this]() { router_->Cleanup(); });

  // Send a RX packet to the router during cleanup. OnTransportPacketReady
  // should return immediately to prevent deadlock.
  HalPacket packet({0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  transport_interface_callback_->OnTransportPacketReady(packet);

  // Unlock cleanup thread after RX, also wait for the thread to complete the
  // task.
  {
    std::lock_guard<std::mutex> lock(m);
    rx_dispatched = true;
  }
  cv.notify_one();
  cleanup_thread.join();

  // Reset CleanupTransport() for TearDown.
  ON_CALL(mock_transport_interface_, CleanupTransport())
      .WillByDefault(Invoke([]() {}));
}

TEST_F(HciRouterTest, HandleTxAfterAnotherThreadCallingCleanup) {
  // The `Cleanup` can be called in a different thread than the thread calling
  // `Send`, `SendCommand`, or `SendCommandNoAck`, e.g. shutdown.
  std::mutex m;
  std::condition_variable cv_cleanup_started;

  // Override CleanupTransport(), force it wait for the send to be attempted.
  ON_CALL(mock_transport_interface_, CleanupTransport())
      .WillByDefault(Invoke([&]() {
        std::lock_guard<std::mutex> lock(m);
        cv_cleanup_started.notify_one();
      }));

  // Start cleaning up the router.
  auto cleanup_thread = std::thread([this]() { router_->Cleanup(); });

  // Wait until the cleanup has actually started.
  {
    std::unique_lock<std::mutex> lock(m);
    auto status = cv_cleanup_started.wait_for(lock, std::chrono::seconds(3));
    EXPECT_EQ(status, std::cv_status::no_timeout);
  }
  HalPacket packet({0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  router_->Send(packet);

  cleanup_thread.join();

  ON_CALL(mock_transport_interface_, CleanupTransport())
      .WillByDefault(Invoke([]() {}));
}

TEST_F(HciRouterTest, HandleReplaceHciEventByClient) {
  // Test a scenario of
  // [Stack] <══( Event A )══  [HAL] <══( Event B )══ [Controller]

  HalPacket event_before_intercept(
      {0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  HalPacket event_after_intercept(
      {0x04, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01});

  // Check if the received event is replaced by a client.
  EXPECT_CALL(mock_hci_router_client_agent_,
              DispatchPacketToClients(event_before_intercept))
      .WillOnce(Invoke([&](const HalPacket& captured_event) {
        EXPECT_EQ(captured_event.GetDestination(), PacketDestination::kHost);
        EXPECT_EQ(captured_event, event_before_intercept);

        // Send the new event to the stack from the client.
        router_->SendPacketToStack(event_after_intercept);
        return MonitorMode::kIntercept;
      }));

  // Expect the event after intercept will still be passed to the router client
  // agent, and expect it to ignore it.
  EXPECT_CALL(mock_hci_router_client_agent_,
              DispatchPacketToClients(event_after_intercept))
      .WillOnce(Invoke([&](const HalPacket&) { return MonitorMode::kBypass; }));

  // Expecting the stack to get the new event.
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(event_after_intercept))
      .Times(1);

  transport_interface_callback_->OnTransportPacketReady(event_before_intercept);
  EXPECT_EQ(hal_packet_, event_after_intercept);
}

TEST_F(HciRouterTest, HandleReplaceHciCommandByClient) {
  // Test a scenario of
  // [Stack] ═══(Command A)══> [HAL] ═══(Command B)══> [Controller]

  set_com_android_bluetooth_bluetooth_hal_flags_handle_recursive_packets_from_router_clients(
      true);

  HalPacket cmd_before_intercept({0x01, 0x03, 0x0c, 0x00});
  HalPacket cmd_after_intercept({0x01, 0x03, 0x0c, 0x01, 0x00});
  cmd_before_intercept.SetSource(PacketSource::kStack);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_before_intercept)).Times(0);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_after_intercept)).Times(1);

  EXPECT_CALL(mock_hci_router_client_agent_,
              DispatchPacketToClients(cmd_before_intercept))
      .WillOnce(Invoke([&](const HalPacket& captured_packet) {
        EXPECT_EQ(captured_packet.GetDestination(),
                  PacketDestination::kController);
        EXPECT_EQ(captured_packet.GetSource(), PacketSource::kStack);
        SendToRouter(cmd_after_intercept);
        return MonitorMode::kIntercept;
      }));

  EXPECT_CALL(mock_hci_router_client_agent_,
              DispatchPacketToClients(cmd_after_intercept))
      .WillOnce(Invoke([&](const HalPacket&) { return MonitorMode::kBypass; }));

  EXPECT_TRUE(SendToRouter(cmd_before_intercept));
  WaitPacketSentToTransport(cmd_after_intercept);
  EXPECT_TRUE(GetIsRouterBusy());
}

TEST_F(HciRouterTest, HandleComplexInterceptScenarioA) {
  // Test a complex scenario of
  // [Stack] ═══(Command A)══> [HAL] ═══(Command B)══> [Controller]
  // [Stack] <══( Event A )══  [HAL] <══( Event B )═════════╝

  set_com_android_bluetooth_bluetooth_hal_flags_handle_recursive_packets_from_router_clients(
      true);

  HalPacket command_A({0x01, 0x03, 0x0c, 0x00});
  HalPacket command_B({0x01, 0x03, 0x0c, 0x01, 0x00});
  HalPacket event_A({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  HalPacket event_B({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x01});

  EXPECT_CALL(mock_transport_interface_, Send(command_A)).Times(0);
  EXPECT_CALL(mock_transport_interface_, Send(command_B)).Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(event_A)).Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(event_B)).Times(0);

  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(command_A))
      .WillOnce(Invoke([&](const HalPacket&) {
        SendCommandToRouter(command_B, [&](const HalPacket&) {
          router_->SendPacketToStack(event_A);
        });
        return MonitorMode::kIntercept;
      }));
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(command_B))
      .WillOnce(Invoke([&](const HalPacket&) { return MonitorMode::kBypass; }));

  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(event_A))
      .WillOnce(Invoke([&](const HalPacket&) { return MonitorMode::kBypass; }));
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(event_B))
      .WillOnce(Invoke([&](const HalPacket&) { return MonitorMode::kNone; }));

  EXPECT_TRUE(SendToRouter(command_A));
  WaitPacketSentToTransport(command_B);
  transport_interface_callback_->OnTransportPacketReady(event_B);
}

TEST_F(HciRouterTest, HandleComplexInterceptScenarioB) {
  // Test a complex scenario of
  // [Stack] ═══(Command A)══> [HAL]
  // [Stack] <══( Event A )═════╣
  //                            ╚═══════(Command B)══> [Controller]
  //                           [HAL] <══( Event B )═════════╝

  set_com_android_bluetooth_bluetooth_hal_flags_handle_recursive_packets_from_router_clients(
      true);

  HalPacket command_A({0x01, 0x03, 0x0c, 0x00});
  HalPacket command_B({0x01, 0x03, 0x0c, 0x01, 0x00});
  HalPacket event_A({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  HalPacket event_B({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x01});

  EXPECT_CALL(mock_transport_interface_, Send(command_A)).Times(0);
  EXPECT_CALL(mock_transport_interface_, Send(command_B)).Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(event_A)).Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(event_B)).Times(0);

  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(command_A))
      .WillOnce(Invoke([&](const HalPacket&) {
        router_->SendPacketToStack(event_A);
        SendCommandToRouter(command_B, [&](const HalPacket& captured_packet) {
          EXPECT_EQ(captured_packet, event_B);
        });
        return MonitorMode::kIntercept;
      }));
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(command_B))
      .WillOnce(Invoke([&](const HalPacket&) { return MonitorMode::kBypass; }));

  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(event_A))
      .WillOnce(Invoke([&](const HalPacket&) { return MonitorMode::kBypass; }));
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(event_B))
      .WillOnce(Invoke([&](const HalPacket&) { return MonitorMode::kNone; }));

  EXPECT_TRUE(SendToRouter(command_A));
  WaitPacketSentToTransport(command_B);
  transport_interface_callback_->OnTransportPacketReady(event_B);
}

}  // namespace
}  // namespace bluetooth_hal::hci
