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

#include <future>
#include <memory>

#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_router_callback.h"
#include "bluetooth_hal/test/mock/mock_hal_config_loader.h"
#include "bluetooth_hal/test/mock/mock_hci_router_client_agent.h"
#include "bluetooth_hal/test/mock/mock_transport_factory.h"
#include "bluetooth_hal/test/mock/mock_transport_interface.h"
#include "bluetooth_hal/test/mock/mock_vnd_snoop_logger.h"
#include "bluetooth_hal/test/mock/mock_wakelock.h"
#include "bluetooth_hal/transport/transport_interface.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal::hci {
namespace {

using ::bluetooth_hal::HalState;
using ::bluetooth_hal::config::MockHalConfigLoader;
using ::bluetooth_hal::debug::MockVndSnoopLogger;
using ::bluetooth_hal::transport::MockTransportFactory;
using ::bluetooth_hal::transport::MockTransportInterface;
using ::bluetooth_hal::transport::TransportInterfaceCallback;
using ::bluetooth_hal::util::power::MockWakelock;
using ::testing::_;
using ::testing::An;
using ::testing::DoAll;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnRef;
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

class MockTransportInterfaceCallback : public TransportInterfaceCallback {
 public:
  MOCK_METHOD(void, OnTransportPacketReady, (const HalPacket& packet),
              (override));
  MOCK_METHOD(void, OnTransportClosed, (), (override));
};

class HciRouterAsyncTest : public Test {
 protected:
  void SetUp() override {
    mock_hci_router_callback_ = std::make_shared<MockHciRouterCallback>();
    mock_transport_interface_callback_ =
        std::make_shared<MockTransportInterfaceCallback>();

    MockHciRouterClientAgent::SetMockAgent(&mock_hci_router_client_agent_);
    MockTransportInterface::SetMockTransport(&mock_transport_interface_);
    MockTransportFactory::SetMockFactory(&mock_transport_factory_);
    MockHalConfigLoader::SetMockLoader(&mock_hal_config_loader_);
    MockWakelock::SetMockWakelock(&mock_wakelock_);
    MockVndSnoopLogger::SetMockVndSnoopLogger(&mock_vnd_snoop_logger_);

    ON_CALL(mock_transport_factory_, GetTransport())
        .WillByDefault(ReturnRef(mock_transport_interface_));
    ON_CALL(mock_transport_factory_, CleanupTransport())
        .WillByDefault(
            Invoke([&mock_transport_interface = mock_transport_interface_]() {
              mock_transport_interface.Cleanup();
            }));

    router_ = std::make_shared<HciRouterAsync>();

    ON_CALL(mock_transport_interface_, IsTransportActive())
        .WillByDefault(Return(true));
    ON_CALL(mock_transport_interface_, Initialize(_))
        .WillByDefault(Invoke(
            [this](TransportInterfaceCallback* transport_interface_callback) {
              this->transport_interface_callback_ =
                  transport_interface_callback;
              return true;
            }));
    ON_CALL(mock_hal_config_loader_, IsAcceleratedBtOnSupported())
        .WillByDefault(Return(false));
    ON_CALL(*mock_hci_router_callback_, OnCommandCallback(_))
        .WillByDefault(Invoke([this](const HalPacket& packet) {
          mock_hci_router_callback_->OnPacketCallback(packet);
        }));

    // Initialize the router
    EXPECT_CALL(mock_transport_interface_, Initialize(_)).Times(1);
    router_->Initialize(mock_hci_router_callback_,
                        mock_transport_interface_callback_.get());
  }

  void TearDown() override {
    router_.reset();
    MockHciRouterClientAgent::SetMockAgent(nullptr);
    MockTransportInterface::SetMockTransport(nullptr);
    MockTransportFactory::SetMockFactory(nullptr);
    MockHalConfigLoader::SetMockLoader(nullptr);
    MockWakelock::SetMockWakelock(nullptr);
    MockVndSnoopLogger::SetMockVndSnoopLogger(nullptr);
    mock_transport_interface_callback_.reset();
  }

  std::shared_ptr<HciRouterAsync> router_;
  std::shared_ptr<MockHciRouterCallback> mock_hci_router_callback_;
  std::shared_ptr<MockTransportInterfaceCallback>
      mock_transport_interface_callback_;
  MockHciRouterClientAgent mock_hci_router_client_agent_;
  MockTransportInterface mock_transport_interface_;
  MockTransportFactory mock_transport_factory_;
  MockHalConfigLoader mock_hal_config_loader_;
  MockWakelock mock_wakelock_;
  MockVndSnoopLogger mock_vnd_snoop_logger_;
  TransportInterfaceCallback* transport_interface_callback_;
};

TEST_F(HciRouterAsyncTest, DoInRouterThread) {
  std::promise<void> promise;
  auto future = promise.get_future();
  bool result =
      router_->DoInRouterThread([&promise]() { promise.set_value(); });
  EXPECT_TRUE(result);
  EXPECT_EQ(future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);
}

TEST_F(HciRouterAsyncTest, SynchronousDoInRouterThread) {
  bool task_executed = false;
  bool result =
      router_->SynchronousDoInRouterThread([&]() { task_executed = true; });
  EXPECT_TRUE(result);
  EXPECT_TRUE(task_executed);
}

TEST_F(HciRouterAsyncTest, SynchronousDoInRouterThreadFromRouterThread) {
  std::promise<void> promise;
  auto future = promise.get_future();

  bool result = router_->DoInRouterThread([this, &promise]() {
    router_->SynchronousDoInRouterThread([&promise]() { promise.set_value(); });
  });
  EXPECT_TRUE(result);
  EXPECT_EQ(future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);
}

TEST_F(HciRouterAsyncTest, SendAclData) {
  HalPacket acl_data({0x01, 0x02, 0x03});
  acl_data.SetSource(PacketSource::kStack);
  EXPECT_CALL(mock_transport_interface_, Send(acl_data)).Times(1);
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(acl_data))
      .WillOnce(Invoke([&](const HalPacket& captured_packet) {
        EXPECT_EQ(captured_packet.GetDestination(),
                  PacketDestination::kController);
        EXPECT_EQ(captured_packet.GetSource(), PacketSource::kStack);
        return MonitorMode::kNone;
      }));

  router_->Send(acl_data);
}

TEST_F(HciRouterAsyncTest, SendHciCommand) {
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

  router_->Send(cmd);
}

TEST_F(HciRouterAsyncTest, SendHciCommandTwiceWithoutEvent) {
  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  cmd_reset.SetSource(PacketSource::kStack);
  HalPacket cmd_set_host_le_support({0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00});
  cmd_set_host_le_support.SetSource(PacketSource::kStack);

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

  router_->Send(cmd_reset);
  router_->Send(cmd_set_host_le_support);
}

TEST_F(HciRouterAsyncTest, SendHciCommandTwiceWithEvent) {
  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  cmd_reset.SetSource(PacketSource::kStack);
  HalPacket evt_reset({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  HalPacket cmd_set_host_le_support({0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00});
  cmd_set_host_le_support.SetSource(PacketSource::kStack);

  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(1);
  EXPECT_CALL(*mock_hci_router_callback_, OnPacketCallback(evt_reset)).Times(1);
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
  router_->Send(cmd_reset);
  // Send the second command.
  router_->Send(cmd_set_host_le_support);
  // Receive the event for the first command, and pass to the stack callback.
  router_->OnTransportPacketReady(evt_reset);
}

TEST_F(HciRouterAsyncTest, SendHciCommandTwiceWithLateEvent) {
  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  cmd_reset.SetSource(PacketSource::kStack);
  HalPacket evt_reset({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  HalPacket cmd_set_host_le_support({0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00});
  cmd_set_host_le_support.SetSource(PacketSource::kStack);
  HalPacket evt_set_host_le_support({0x04, 0x0e, 0x04, 0x01, 0x6d, 0x0c, 0x00});

  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(1);
  EXPECT_CALL(*mock_hci_router_callback_, OnPacketCallback(_)).Times(2);

  // Send the first command.
  router_->Send(cmd_reset);
  // Send the second command.
  router_->Send(cmd_set_host_le_support);
  // Receive the event for the first command, and pass to the stack callback.
  router_->OnTransportPacketReady(evt_reset);
  // Receive the event for the second command.
  router_->OnTransportPacketReady(evt_set_host_le_support);
}

TEST_F(HciRouterAsyncTest, SendCommandTwiceWithoutEvent) {
  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  cmd_reset.SetSource(PacketSource::kStack);
  HalPacket cmd_set_host_le_support({0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00});
  cmd_set_host_le_support.SetSource(PacketSource::kStack);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(0);

  router_->SendCommand(
      cmd_reset, std::make_shared<HalPacketCallback>([](const HalPacket&) {}));
  router_->SendCommand(
      cmd_set_host_le_support,
      std::make_shared<HalPacketCallback>([](const HalPacket&) {}));
}

TEST_F(HciRouterAsyncTest, SendCommandTwiceWithEvent) {
  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  cmd_reset.SetSource(PacketSource::kStack);
  HalPacket evt_reset({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  HalPacket cmd_set_host_le_support({0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00});
  cmd_set_host_le_support.SetSource(PacketSource::kStack);

  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(1);
  EXPECT_CALL(*mock_hci_router_callback_, OnPacketCallback(_)).Times(0);

  HalPacket event;
  auto callback = std::make_shared<HalPacketCallback>(
      [&event](const HalPacket& packet) { event = packet; });

  // Send the first command with a client callback.
  router_->SendCommand(cmd_reset, callback);
  // Receive the event for the first command, check if the event is sent to
  // the client callback.
  router_->OnTransportPacketReady(evt_reset);
  EXPECT_EQ(event, evt_reset);
  // Send the second command.
  router_->SendCommand(
      cmd_set_host_le_support,
      std::make_shared<HalPacketCallback>([](const HalPacket&) {}));
}

TEST_F(HciRouterAsyncTest, SendCommandTwiceWithLateEvent) {
  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  cmd_reset.SetSource(PacketSource::kStack);
  HalPacket evt_reset({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  HalPacket cmd_set_host_le_support({0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00});
  cmd_set_host_le_support.SetSource(PacketSource::kStack);
  HalPacket evt_set_host_le_support({0x04, 0x0e, 0x04, 0x01, 0x6d, 0x0c, 0x00});

  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(1);
  EXPECT_CALL(*mock_hci_router_callback_, OnPacketCallback(_)).Times(0);

  HalPacket event;
  auto callback = std::make_shared<HalPacketCallback>(
      [&event](const HalPacket& packet) { event = packet; });

  // Send the first command with a client callback.
  router_->SendCommand(cmd_reset, callback);
  // Send the second command.
  router_->SendCommand(
      cmd_set_host_le_support,
      std::make_shared<HalPacketCallback>([](const HalPacket&) {}));
  // Receive the event for the first command, check if the event is sent to
  // the client callback.
  router_->OnTransportPacketReady(evt_reset);
  EXPECT_EQ(event, evt_reset);

  // Receive the event for the second command.
  router_->OnTransportPacketReady(evt_set_host_le_support);
}

TEST_F(HciRouterAsyncTest, SendHciCommandInCallback) {
  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  cmd_reset.SetSource(PacketSource::kStack);
  HalPacket evt_reset({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  HalPacket cmd_set_host_le_support({0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00});
  cmd_set_host_le_support.SetSource(PacketSource::kStack);
  HalPacket evt_set_host_le_support({0x04, 0x0e, 0x04, 0x01, 0x6d, 0x0c, 0x00});

  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(1);
  EXPECT_CALL(*mock_hci_router_callback_, OnPacketCallback(_)).Times(0);

  HalPacket event;
  auto callback =
      std::make_shared<HalPacketCallback>([&](const HalPacket& packet) {
        event = packet;
        router_->SendCommand(
            cmd_set_host_le_support,
            std::make_shared<HalPacketCallback>([](const HalPacket&) {}));
      });

  // Send the first command with a client callback, and send the second
  // command in the client callback.
  router_->SendCommand(cmd_reset, callback);
  // Receive the generated event for the first command, check if the second
  // command is properly sent.
  router_->OnTransportPacketReady(evt_reset);
  EXPECT_EQ(event, evt_reset);
  // Check the second command is properly handled.
  router_->OnTransportPacketReady(evt_set_host_le_support);
}

TEST_F(HciRouterAsyncTest, SendHciCommandInCallbackAfterAnotherSendCommand) {
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

  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_min_enc_key_size))
      .Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(1);
  EXPECT_CALL(*mock_hci_router_callback_, OnPacketCallback(_)).Times(0);

  HalPacket event;
  auto callback_reset =
      std::make_shared<HalPacketCallback>([&](const HalPacket& packet) {
        event = packet;
        router_->SendCommand(
            cmd_set_host_le_support,
            std::make_shared<HalPacketCallback>([](const HalPacket&) {}));
      });
  auto callback_min_enc_key_size = std::make_shared<HalPacketCallback>(
      [&event](const HalPacket& packet) { event = packet; });

  // Send the first command with a client callback, and send the second
  // command in the client callback.
  router_->SendCommand(cmd_reset, callback_reset);
  router_->SendCommand(cmd_set_min_enc_key_size, callback_min_enc_key_size);

  // Receive three generated events in order
  router_->OnTransportPacketReady(evt_reset);
  EXPECT_EQ(event, evt_reset);

  router_->OnTransportPacketReady(evt_set_min_enc_key_size);
  EXPECT_EQ(event, evt_set_min_enc_key_size);

  router_->OnTransportPacketReady(evt_set_host_le_support);
}

TEST_F(HciRouterAsyncTest, SendDebugInfoCommandAfterSendCommand) {
  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  cmd_reset.SetSource(PacketSource::kStack);
  HalPacket cmd_debug_info({0x01, 0x5B, 0xFD, 0x00});
  cmd_debug_info.SetSource(PacketSource::kStack);

  // Expect both cmd_reset and cmd_debug_info are sent to the transport layer.
  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_debug_info)).Times(1);

  router_->SendCommand(
      cmd_reset, std::make_shared<HalPacketCallback>([](const HalPacket&) {}));
  router_->SendCommand(cmd_debug_info, std::make_shared<HalPacketCallback>(
                                           [](const HalPacket&) {}));
}

TEST_F(HciRouterAsyncTest, SendCommandNoAck) {
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

  router_->SendCommandNoAck(cmd_reset);
  router_->SendCommandNoAck(cmd_set_host_le_support);
}

TEST_F(HciRouterAsyncTest, DispatchPacketToClientsMonitorNone) {
  HalPacket event({0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});

  // Check if the received event is dispatched to client agent and stack.
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(event))
      .WillOnce(Invoke([&](const HalPacket& captured_event) {
        EXPECT_EQ(captured_event.GetDestination(), PacketDestination::kHost);
        return MonitorMode::kNone;
      }));
  EXPECT_CALL(*mock_hci_router_callback_, OnPacketCallback(event)).Times(1);

  router_->OnTransportPacketReady(event);
}

TEST_F(HciRouterAsyncTest, DispatchPacketToClientsMonitorMonitor) {
  HalPacket event({0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});

  // Check if the received event is dispatched to client agent and stack.
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(event))
      .WillOnce(Invoke([&](const HalPacket& captured_event) {
        EXPECT_EQ(captured_event.GetDestination(), PacketDestination::kHost);
        return MonitorMode::kMonitor;
      }));
  EXPECT_CALL(*mock_hci_router_callback_, OnPacketCallback(event)).Times(1);

  router_->OnTransportPacketReady(event);
}

TEST_F(HciRouterAsyncTest, DispatchPacketToClientsMonitorIntercept) {
  HalPacket event({0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});

  // Check if the received event is only dispatched to the agent.
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(event))
      .WillOnce(Invoke([&](const HalPacket& captured_event) {
        EXPECT_EQ(captured_event.GetDestination(), PacketDestination::kHost);
        EXPECT_EQ(captured_event.GetSource(), PacketSource::kController);
        return MonitorMode::kIntercept;
      }));
  EXPECT_CALL(*mock_hci_router_callback_, OnPacketCallback(_)).Times(0);

  router_->OnTransportPacketReady(event);
}

TEST_F(HciRouterAsyncTest, OnAclDataCallback) {
  EXPECT_CALL(*mock_hci_router_callback_, OnPacketCallback(_)).Times(1);
  HalPacket packet(static_cast<uint8_t>(HciPacketType::kAclData),
                   {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  router_->OnTransportPacketReady(packet);
}

TEST_F(HciRouterAsyncTest, OnScoDataCallback) {
  EXPECT_CALL(*mock_hci_router_callback_, OnPacketCallback(_)).Times(1);
  HalPacket packet(static_cast<uint8_t>(HciPacketType::kScoData),
                   {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  router_->OnTransportPacketReady(packet);
}

TEST_F(HciRouterAsyncTest, OnIsoDataCallback) {
  EXPECT_CALL(*mock_hci_router_callback_, OnPacketCallback(_)).Times(1);
  HalPacket packet(static_cast<uint8_t>(HciPacketType::kIsoData),
                   {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  router_->OnTransportPacketReady(packet);
}

TEST_F(HciRouterAsyncTest, DispatchPacketToClientsInterceptThreadData) {
  HalPacket thread_data({0x70, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});

  ON_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(_))
      .WillByDefault(Return(MonitorMode::kIntercept));

  // Expect router callback is called, but stack callback is not.
  EXPECT_CALL(mock_hci_router_client_agent_,
              DispatchPacketToClients(thread_data))
      .Times(1);
  EXPECT_CALL(*mock_hci_router_callback_, OnPacketCallback(_)).Times(0);

  router_->OnTransportPacketReady(thread_data);
}

TEST_F(HciRouterAsyncTest, SendPacketToStack) {
  EXPECT_CALL(*mock_hci_router_callback_, OnPacketCallback(_)).Times(1);
  HalPacket packet({0x02, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  router_->SendPacketToStack(packet);
}

TEST_F(HciRouterAsyncTest, UpdateHalState) {
  // Tests if the HalState change is notified to callbacks and agents.
  EXPECT_CALL(
      *mock_hci_router_callback_,
      OnHalStateChanged(HalState::kPreFirmwareDownload, HalState::kInit))
      .Times(1);
  EXPECT_CALL(
      mock_hci_router_client_agent_,
      NotifyHalStateChange(HalState::kPreFirmwareDownload, HalState::kInit))
      .Times(1);
  EXPECT_CALL(mock_transport_factory_,
              NotifyHalStateChange(HalState::kPreFirmwareDownload))
      .Times(1);

  router_->UpdateHalState(HalState::kPreFirmwareDownload);
}

TEST_F(HciRouterAsyncTest, ReplaceHciEventByClient) {
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

  EXPECT_CALL(*mock_hci_router_callback_,
              OnPacketCallback(event_after_intercept))
      .Times(1);

  router_->OnTransportPacketReady(event_before_intercept);
}

TEST_F(HciRouterAsyncTest, ReplaceHciCommandByClient) {
  // Test a scenario of
  // [Stack] ═══(Command A)══> [HAL] ═══(Command B)══> [Controller]

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
        router_->Send(cmd_after_intercept);
        return MonitorMode::kIntercept;
      }));

  EXPECT_CALL(mock_hci_router_client_agent_,
              DispatchPacketToClients(cmd_after_intercept))
      .WillOnce(Invoke([&](const HalPacket&) { return MonitorMode::kBypass; }));

  router_->Send(cmd_before_intercept);
}

TEST_F(HciRouterAsyncTest, ComplexInterceptScenarioA) {
  // Test a complex scenario of
  // [Stack] ═══(Command A)══> [HAL] ═══(Command B)══> [Controller]
  // [Stack] <══( Event A )══  [HAL] <══( Event B )═════════╝

  HalPacket command_A({0x01, 0x03, 0x0c, 0x00});
  HalPacket command_B({0x01, 0x03, 0x0c, 0x01, 0x00});
  HalPacket event_A({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  HalPacket event_B({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x01});

  EXPECT_CALL(mock_transport_interface_, Send(command_A)).Times(0);
  EXPECT_CALL(mock_transport_interface_, Send(command_B)).Times(1);
  EXPECT_CALL(*mock_hci_router_callback_, OnPacketCallback(event_A)).Times(1);
  EXPECT_CALL(*mock_hci_router_callback_, OnPacketCallback(event_B)).Times(0);

  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(command_A))
      .WillOnce(Invoke([&](const HalPacket&) {
        // Send command_B with a callback that sends event_A to stack
        auto callback = std::make_shared<HalPacketCallback>(
            [&](const HalPacket&) { router_->SendPacketToStack(event_A); });
        router_->SendCommand(command_B, callback);
        return MonitorMode::kIntercept;
      }));
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(command_B))
      .WillOnce(Invoke([&](const HalPacket&) { return MonitorMode::kBypass; }));

  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(event_B))
      .WillOnce(Invoke([&](const HalPacket&) { return MonitorMode::kNone; }));

  router_->Send(command_A);
  // Simulate receiving event_B which triggers the callback for command_B
  router_->OnTransportPacketReady(event_B);
}

TEST_F(HciRouterAsyncTest, ComplexInterceptScenarioB) {
  // Test a complex scenario of
  // [Stack] ═══(Command A)══> [HAL]
  // [Stack] <══( Event A )═════╣
  //                            ╚═══════(Command B)══> [Controller]
  //                           [HAL] <══( Event B )═════════╝

  HalPacket command_A({0x01, 0x03, 0x0c, 0x00});
  HalPacket command_B({0x01, 0x03, 0x0c, 0x01, 0x00});
  HalPacket event_A({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  HalPacket event_B({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x01});

  EXPECT_CALL(mock_transport_interface_, Send(command_A)).Times(0);
  EXPECT_CALL(mock_transport_interface_, Send(command_B)).Times(1);
  EXPECT_CALL(*mock_hci_router_callback_, OnPacketCallback(event_A)).Times(1);
  EXPECT_CALL(*mock_hci_router_callback_, OnPacketCallback(event_B)).Times(0);

  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(command_A))
      .WillOnce(Invoke([&](const HalPacket&) {
        router_->SendPacketToStack(event_A);
        auto callback = std::make_shared<HalPacketCallback>(
            [&](const HalPacket& captured_packet) {
              EXPECT_EQ(captured_packet, event_B);
            });
        router_->SendCommand(command_B, callback);
        return MonitorMode::kIntercept;
      }));
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(command_B))
      .WillOnce(Invoke([&](const HalPacket&) { return MonitorMode::kBypass; }));

  // No expectation for dispatch of event_A

  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(event_B))
      .WillOnce(Invoke([&](const HalPacket&) { return MonitorMode::kNone; }));

  router_->Send(command_A);
  // Simulate receiving event_B which triggers the callback for command_B
  router_->OnTransportPacketReady(event_B);
}

}  // namespace
}  // namespace bluetooth_hal::hci
