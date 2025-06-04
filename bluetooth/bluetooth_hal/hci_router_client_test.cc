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

#include "bluetooth_hal/hci_router_client.h"

#include <list>
#include <utility>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_monitor.h"
#include "bluetooth_hal/test/mock/mock_hci_router.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace hci {
namespace {

using ::bluetooth_hal::HalState;

using ::testing::_;
using ::testing::Return;
using ::testing::Test;

class HciRouterClientTestInstance : public HciRouterClient {
 public:
  MOCK_METHOD(void, OnBluetoothChipReady, (), (override));
  MOCK_METHOD(void, OnBluetoothChipClosed, (), (override));
  MOCK_METHOD(void, OnBluetoothEnabled, (), (override));
  MOCK_METHOD(void, OnBluetoothDisabled, (), (override));

  void OnCommandCallback([[maybe_unused]] const HalPacket& packet) override {}

  void OnMonitorPacketCallback(MonitorMode mode,
                               const HalPacket& packet) override {
    on_monitor_callbacks_.push_back(std::pair(mode, packet));
  }

  // Wrappers to access protected methods
  bool IsBluetoothChipReadyWrapper() { return IsBluetoothChipReady(); }

  bool RegisterMonitorWrapper(const HciMonitor& monitor, MonitorMode mode) {
    return RegisterMonitor(monitor, mode);
  }

  bool UnregisterMonitorWrapper(const HciMonitor& monitor) {
    return UnregisterMonitor(monitor);
  }

  bool SendCommandWrapper(const HalPacket& packet) {
    return SendCommand(packet);
  }

  bool SendDataWrapper(const HalPacket& packet) { return SendData(packet); }

  bool IsBluetoothEnabledWrapper() { return IsBluetoothEnabled(); }

  std::list<std::pair<MonitorMode, HalPacket>> on_monitor_callbacks_;
};

class HciRouterClientTest : public Test {
 protected:
  static void SetUpTestSuite() {}

  void SetUp() override {
    MockHciRouter::SetMockRouter(&mock_hci_router_);

    ON_CALL(mock_hci_router_, Send(_)).WillByDefault(Return(true));
    ON_CALL(mock_hci_router_, SendCommand(_, _)).WillByDefault(Return(true));
    ON_CALL(mock_hci_router_, RegisterCallback(_)).WillByDefault(Return(true));
    ON_CALL(mock_hci_router_, UnregisterCallback(_))
        .WillByDefault(Return(true));
    EXPECT_CALL(mock_hci_router_, RegisterCallback(_)).Times(1);

    router_client_ = new HciRouterClientTestInstance();
  }

  void TearDown() override {
    EXPECT_CALL(mock_hci_router_, UnregisterCallback(_)).Times(1);
    delete (router_client_);
  }

  HalPacket GenerateHciResetCommand() {
    return HalPacket(std::vector<uint8_t>{0x01, 0x03, 0x0C, 0x00});
  }

  HalPacket GenerateHciResetCompleteEvent() {
    return HalPacket(
        std::vector<uint8_t>{0x04, 0x0E, 0x04, 0x01, 0x03, 0x0C, 0x00});
  }

  HalPacket GenerateBleAdvReportEvent() {
    return HalPacket(std::vector<uint8_t>{
        0x04, 0x3E, 0x1D, 0x0D, 0x01, 0x12, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01, 0x00, 0xFF, 0x7F, 0xC0, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x01, 0x02});
  }

  HalPacket GenerateRandomPacket() {
    return HalPacket(std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                                          0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
                                          0x0D, 0x0E, 0x0F});
  }

  void TestHandleRegisterMonitor(HciMonitor monitor, MonitorMode mode,
                                 HalPacket packet, int expect_call_count) {
    HalPacket packet_random = GenerateRandomPacket();

    ASSERT_TRUE(router_client_->RegisterMonitorWrapper(monitor, mode));
    ASSERT_EQ(router_client_->OnPacketCallback(packet), mode);
    ASSERT_EQ(router_client_->on_monitor_callbacks_.size(), expect_call_count);
    ASSERT_EQ(router_client_->OnPacketCallback(packet_random),
              MonitorMode::kNone);
    ASSERT_EQ(router_client_->on_monitor_callbacks_.size(), expect_call_count);
    ASSERT_TRUE(router_client_->UnregisterMonitorWrapper(monitor));
    ASSERT_EQ(router_client_->OnPacketCallback(packet), MonitorMode::kNone);
    ASSERT_EQ(router_client_->on_monitor_callbacks_.size(), expect_call_count);
    ASSERT_EQ(router_client_->OnPacketCallback(packet_random),
              MonitorMode::kNone);
    ASSERT_EQ(router_client_->on_monitor_callbacks_.size(), expect_call_count);
  }

  HciRouterClientTestInstance* router_client_;
  MockHciRouter mock_hci_router_;
  static constexpr uint16_t kHciResetCommandOpcode = 0x0C03;
  static constexpr uint16_t kHciBleAdvSubCode = 0x0D;
};

TEST_F(HciRouterClientTest, HandleInit) {
  ASSERT_FALSE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_FALSE(router_client_->IsBluetoothEnabledWrapper());
}

TEST_F(HciRouterClientTest, HandleOnHalStateChangedShutdownToInit) {
  EXPECT_CALL(*router_client_, OnBluetoothChipReady()).Times(0);
  EXPECT_CALL(*router_client_, OnBluetoothChipClosed()).Times(0);
  EXPECT_CALL(*router_client_, OnBluetoothEnabled()).Times(0);
  EXPECT_CALL(*router_client_, OnBluetoothDisabled()).Times(0);
  ASSERT_FALSE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_FALSE(router_client_->IsBluetoothEnabledWrapper());
  router_client_->OnHalStateChanged(HalState::kInit, HalState::kShutdown);
  ASSERT_FALSE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_FALSE(router_client_->IsBluetoothEnabledWrapper());
}

TEST_F(HciRouterClientTest, HandleOnHalStateChangedInitToFirmwareDownloading) {
  EXPECT_CALL(*router_client_, OnBluetoothChipReady()).Times(0);
  EXPECT_CALL(*router_client_, OnBluetoothChipClosed()).Times(0);
  EXPECT_CALL(*router_client_, OnBluetoothEnabled()).Times(0);
  EXPECT_CALL(*router_client_, OnBluetoothDisabled()).Times(0);
  ASSERT_FALSE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_FALSE(router_client_->IsBluetoothEnabledWrapper());
  router_client_->OnHalStateChanged(HalState::kFirmwareDownloading,
                                    HalState::kInit);
  ASSERT_FALSE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_FALSE(router_client_->IsBluetoothEnabledWrapper());
}

TEST_F(HciRouterClientTest,
       HandleOnHalStateChangedFirmwaredownloadingToFirmwaredownloadCompleted) {
  EXPECT_CALL(*router_client_, OnBluetoothChipReady()).Times(0);
  EXPECT_CALL(*router_client_, OnBluetoothChipClosed()).Times(0);
  EXPECT_CALL(*router_client_, OnBluetoothEnabled()).Times(0);
  EXPECT_CALL(*router_client_, OnBluetoothDisabled()).Times(0);
  ASSERT_FALSE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_FALSE(router_client_->IsBluetoothEnabledWrapper());
  router_client_->OnHalStateChanged(HalState::kFirmwareDownloadCompleted,
                                    HalState::kFirmwareDownloading);
  ASSERT_FALSE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_FALSE(router_client_->IsBluetoothEnabledWrapper());
}

TEST_F(HciRouterClientTest,
       HandleOnHalStateChangedFirmwaredownloadCompletedToFirmwareReady) {
  EXPECT_CALL(*router_client_, OnBluetoothChipReady()).Times(0);
  EXPECT_CALL(*router_client_, OnBluetoothChipClosed()).Times(0);
  EXPECT_CALL(*router_client_, OnBluetoothEnabled()).Times(0);
  EXPECT_CALL(*router_client_, OnBluetoothDisabled()).Times(0);
  ASSERT_FALSE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_FALSE(router_client_->IsBluetoothEnabledWrapper());
  router_client_->OnHalStateChanged(HalState::kFirmwareReady,
                                    HalState::kFirmwareDownloadCompleted);
  ASSERT_FALSE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_FALSE(router_client_->IsBluetoothEnabledWrapper());
}

TEST_F(HciRouterClientTest, HandleOnHalStateChangedFirmwareReadyToBtChipReady) {
  EXPECT_CALL(*router_client_, OnBluetoothChipReady()).Times(1);
  EXPECT_CALL(*router_client_, OnBluetoothChipClosed()).Times(0);
  EXPECT_CALL(*router_client_, OnBluetoothEnabled()).Times(0);
  EXPECT_CALL(*router_client_, OnBluetoothDisabled()).Times(0);
  ASSERT_FALSE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_FALSE(router_client_->IsBluetoothEnabledWrapper());
  router_client_->OnHalStateChanged(HalState::kBtChipReady,
                                    HalState::kFirmwareReady);
  ASSERT_TRUE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_FALSE(router_client_->IsBluetoothEnabledWrapper());
}

TEST_F(HciRouterClientTest, HandleOnHalStateChangedBtChipReadyToRunning) {
  EXPECT_CALL(*router_client_, OnBluetoothChipReady()).Times(1);
  EXPECT_CALL(*router_client_, OnBluetoothChipClosed()).Times(0);
  EXPECT_CALL(*router_client_, OnBluetoothEnabled()).Times(0);
  EXPECT_CALL(*router_client_, OnBluetoothDisabled()).Times(0);
  ASSERT_FALSE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_FALSE(router_client_->IsBluetoothEnabledWrapper());
  router_client_->OnHalStateChanged(HalState::kRunning, HalState::kBtChipReady);

  // No reset complete event in this test case. The state is kRunning but no
  // OnBluetoothEnabled callback.
  ASSERT_TRUE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_FALSE(router_client_->IsBluetoothEnabledWrapper());
}

TEST_F(HciRouterClientTest,
       HandleOnHalStateChangedBtChipReadyToRunningWithReset) {
  const HalPacket reset_packet = GenerateHciResetCompleteEvent();

  EXPECT_CALL(*router_client_, OnBluetoothChipReady()).Times(1);
  EXPECT_CALL(*router_client_, OnBluetoothChipClosed()).Times(0);
  EXPECT_CALL(*router_client_, OnBluetoothEnabled()).Times(1);
  EXPECT_CALL(*router_client_, OnBluetoothDisabled()).Times(0);

  ASSERT_FALSE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_FALSE(router_client_->IsBluetoothEnabledWrapper());
  ON_CALL(mock_hci_router_, GetHalState())
      .WillByDefault(Return(HalState::kRunning));
  router_client_->OnHalStateChanged(HalState::kRunning, HalState::kBtChipReady);

  // Send reset complete event to trigger OnBluetoothEnabled.
  ASSERT_EQ(router_client_->OnPacketCallback(reset_packet), MonitorMode::kNone);
  ASSERT_TRUE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_TRUE(router_client_->IsBluetoothEnabledWrapper());
}

TEST_F(HciRouterClientTest, HandleRunningStateWithMultipleReset) {
  const HalPacket reset_packet = GenerateHciResetCompleteEvent();

  EXPECT_CALL(*router_client_, OnBluetoothChipReady()).Times(1);
  EXPECT_CALL(*router_client_, OnBluetoothChipClosed()).Times(0);
  EXPECT_CALL(*router_client_, OnBluetoothEnabled()).Times(1);
  EXPECT_CALL(*router_client_, OnBluetoothDisabled()).Times(0);

  ASSERT_FALSE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_FALSE(router_client_->IsBluetoothEnabledWrapper());
  ON_CALL(mock_hci_router_, GetHalState())
      .WillByDefault(Return(HalState::kRunning));
  router_client_->OnHalStateChanged(HalState::kRunning, HalState::kBtChipReady);

  // Send two reset complete events. OnBluetoothEnabled and OnBluetoothChipReady
  // should only be invoked once for each.
  ASSERT_EQ(router_client_->OnPacketCallback(reset_packet), MonitorMode::kNone);
  ASSERT_EQ(router_client_->OnPacketCallback(reset_packet), MonitorMode::kNone);
  ASSERT_TRUE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_TRUE(router_client_->IsBluetoothEnabledWrapper());
}

TEST_F(HciRouterClientTest,
       HandleOnHalStateChangedRunningToBtChipReadyToShutdown) {
  const HalPacket reset_packet = GenerateHciResetCompleteEvent();

  EXPECT_CALL(*router_client_, OnBluetoothChipReady()).Times(1);
  EXPECT_CALL(*router_client_, OnBluetoothChipClosed()).Times(1);
  EXPECT_CALL(*router_client_, OnBluetoothEnabled()).Times(1);
  EXPECT_CALL(*router_client_, OnBluetoothDisabled()).Times(1);
  ASSERT_FALSE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_FALSE(router_client_->IsBluetoothEnabledWrapper());

  // Turn on Bluetooth and BT chip, check if both Bluetooth enabled and Chip
  // enabled flags are true.
  ON_CALL(mock_hci_router_, GetHalState())
      .WillByDefault(Return(HalState::kRunning));
  router_client_->OnHalStateChanged(HalState::kRunning, HalState::kBtChipReady);
  ASSERT_EQ(router_client_->OnPacketCallback(reset_packet), MonitorMode::kNone);
  ASSERT_TRUE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_TRUE(router_client_->IsBluetoothEnabledWrapper());

  // Turn off Bluetooth and check if the Bluetooth enabled flag is false and the
  // Chip enabled flag is still true.
  router_client_->OnHalStateChanged(HalState::kBtChipReady, HalState::kRunning);
  ASSERT_TRUE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_FALSE(router_client_->IsBluetoothEnabledWrapper());

  // Shutdown Bluetooth HAL, check if both Bluetooth enabled and Chip enabled
  // flags are false.
  router_client_->OnHalStateChanged(HalState::kShutdown,
                                    HalState::kBtChipReady);
  ASSERT_FALSE(router_client_->IsBluetoothChipReadyWrapper());
  ASSERT_FALSE(router_client_->IsBluetoothEnabledWrapper());
}

TEST_F(HciRouterClientTest, HandleSendCommandWithValidInput) {
  const HalPacket packet = GenerateHciResetCommand();
  EXPECT_CALL(mock_hci_router_, SendCommand(packet, _)).Times(1);
  ASSERT_TRUE(router_client_->SendCommandWrapper(packet));
}

TEST_F(HciRouterClientTest, HandleSendCommandWithInValidInput) {
  const HalPacket packet({0x70, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00});
  EXPECT_CALL(mock_hci_router_, SendCommand(packet, _)).Times(0);
  ASSERT_FALSE(router_client_->SendCommandWrapper(packet));
}

TEST_F(HciRouterClientTest, HandleSendDataWithValidInput) {
  const HalPacket packet({0x70, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00});
  EXPECT_CALL(mock_hci_router_, Send(packet)).Times(1);
  ASSERT_TRUE(router_client_->SendDataWrapper(packet));
}

TEST_F(HciRouterClientTest, HandleSendDataWithInValidInput) {
  const HalPacket packet = GenerateHciResetCommand();
  EXPECT_CALL(mock_hci_router_, Send(packet)).Times(0);
  ASSERT_FALSE(router_client_->SendDataWrapper(packet));
}

TEST_F(HciRouterClientTest, HandleRegisterMonitorCommandMonitor) {
  HalPacket packet = GenerateHciResetCommand();
  HciCommandMonitor monitor(kHciResetCommandOpcode);
  MonitorMode mode = MonitorMode::kMonitor;
  TestHandleRegisterMonitor(monitor, mode, packet, 1);
}

TEST_F(HciRouterClientTest, HandleRegisterMonitorCommandIntercept) {
  HalPacket packet = GenerateHciResetCommand();
  HciCommandMonitor monitor(kHciResetCommandOpcode);
  MonitorMode mode = MonitorMode::kIntercept;
  TestHandleRegisterMonitor(monitor, mode, packet, 1);
}

TEST_F(HciRouterClientTest, HandleRegisterMonitorEventMonitor) {
  HalPacket packet = GenerateHciResetCompleteEvent();
  HciEventMonitor monitor(static_cast<uint8_t>(EventCode::kCommandComplete));
  MonitorMode mode = MonitorMode::kMonitor;
  TestHandleRegisterMonitor(monitor, mode, packet, 1);
}

TEST_F(HciRouterClientTest, HandleRegisterMonitorEventIntercept) {
  HalPacket packet = GenerateHciResetCompleteEvent();
  HciEventMonitor monitor(static_cast<uint8_t>(EventCode::kCommandComplete));
  MonitorMode mode = MonitorMode::kIntercept;
  TestHandleRegisterMonitor(monitor, mode, packet, 1);
}

TEST_F(HciRouterClientTest, HandleRegisterMonitorEventMonitorWithEventCode) {
  HalPacket packet = GenerateBleAdvReportEvent();
  HciEventMonitor monitor(static_cast<uint8_t>(EventCode::kBleMeta),
                          kHciBleAdvSubCode,
                          HciConstants::kHciBleEventSubCodeOffset);
  MonitorMode mode = MonitorMode::kMonitor;
  TestHandleRegisterMonitor(monitor, mode, packet, 1);
}

TEST_F(HciRouterClientTest, HandleRegisterMonitorEventInterceptWithEventCode) {
  HalPacket packet = GenerateBleAdvReportEvent();
  HciEventMonitor monitor(static_cast<uint8_t>(EventCode::kBleMeta),
                          kHciBleAdvSubCode,
                          HciConstants::kHciBleEventSubCodeOffset);
  MonitorMode mode = MonitorMode::kIntercept;
  TestHandleRegisterMonitor(monitor, mode, packet, 1);
}

TEST_F(HciRouterClientTest, HandleRegisterMonitorHasOverlapDifferentMode) {
  HalPacket packet = GenerateHciResetCompleteEvent();
  HalPacket packet_random = GenerateRandomPacket();

  HciEventMonitor monitor1(static_cast<uint8_t>(EventCode::kCommandComplete));
  HciCommandCompleteEventMonitor monitor2(kHciResetCommandOpcode);

  ASSERT_TRUE(
      router_client_->RegisterMonitorWrapper(monitor1, MonitorMode::kMonitor));
  ASSERT_TRUE(router_client_->RegisterMonitorWrapper(monitor2,
                                                     MonitorMode::kIntercept));
  ASSERT_EQ(router_client_->OnPacketCallback(packet), MonitorMode::kIntercept);
  ASSERT_EQ(router_client_->on_monitor_callbacks_.size(), 1);

  ASSERT_EQ(router_client_->OnPacketCallback(packet_random),
            MonitorMode::kNone);
  ASSERT_EQ(router_client_->on_monitor_callbacks_.size(), 1);
}

TEST_F(HciRouterClientTest, HandleRegisterMonitorHasOverlapSameMode) {
  HalPacket packet = GenerateHciResetCompleteEvent();
  HalPacket packet_random = GenerateRandomPacket();

  HciEventMonitor monitor1(static_cast<uint8_t>(EventCode::kCommandComplete));
  HciCommandCompleteEventMonitor monitor2(kHciResetCommandOpcode);

  ASSERT_TRUE(
      router_client_->RegisterMonitorWrapper(monitor1, MonitorMode::kMonitor));
  ASSERT_TRUE(
      router_client_->RegisterMonitorWrapper(monitor2, MonitorMode::kMonitor));
  ASSERT_EQ(router_client_->OnPacketCallback(packet), MonitorMode::kMonitor);
  ASSERT_EQ(router_client_->on_monitor_callbacks_.size(), 1);

  ASSERT_EQ(router_client_->OnPacketCallback(packet_random),
            MonitorMode::kNone);
  ASSERT_EQ(router_client_->on_monitor_callbacks_.size(), 1);
}

TEST_F(HciRouterClientTest, HandleRegisterMonitorWithModeNone) {
  HciCommandCompleteEventMonitor monitor(kHciResetCommandOpcode);
  ASSERT_FALSE(
      router_client_->RegisterMonitorWrapper(monitor, MonitorMode::kNone));
}

TEST_F(HciRouterClientTest, HandleRegisterMonitorDoubleRegister) {
  HciCommandCompleteEventMonitor monitor1(kHciResetCommandOpcode);
  HciCommandCompleteEventMonitor monitor2(kHciResetCommandOpcode);
  ASSERT_TRUE(
      router_client_->RegisterMonitorWrapper(monitor1, MonitorMode::kMonitor));
  ASSERT_FALSE(
      router_client_->RegisterMonitorWrapper(monitor2, MonitorMode::kMonitor));
}

TEST_F(HciRouterClientTest, HandleRegisterMonitorUnregisterWithoutRegister) {
  HciCommandCompleteEventMonitor monitor(kHciResetCommandOpcode);
  ASSERT_FALSE(router_client_->UnregisterMonitorWrapper(monitor));
}

}  // namespace
}  // namespace hci
}  // namespace bluetooth_hal
