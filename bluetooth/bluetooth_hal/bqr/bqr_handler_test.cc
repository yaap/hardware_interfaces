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

#include "bluetooth_hal/bqr/bqr_handler.h"

#include <cstdint>
#include <memory>
#include <vector>

#include "android-base/macros.h"
#include "bluetooth_hal/bqr/bqr_event.h"
#include "bluetooth_hal/bqr/bqr_root_inflammation_event.h"
#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/debug/debug_central.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hci_router_client_callback.h"
#include "bluetooth_hal/test/mock/mock_debug_central.h"
#include "bluetooth_hal/test/mock/mock_hci_router_client_agent.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal::bqr {
namespace {

using ::testing::_;
using ::testing::DoAll;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::Test;
using ::testing::TestWithParam;
using ::testing::Values;

using ::bluetooth_hal::debug::MockDebugCentral;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciRouterClientCallback;
using ::bluetooth_hal::hci::MockHciRouterClientAgent;

// Helper functions to create packets
HalPacket CreateVendorCapabilityEvent(BqrVersion version) {
  uint8_t version_byte = 0x00;
  switch (version) {
    case BqrVersion::kV1ToV3:
      version_byte = 0x01;
      break;
    case BqrVersion::kV4:
      version_byte = 0x02;
      break;
    case BqrVersion::kV5:
      version_byte = 0x03;
      break;
    case BqrVersion::kV6:
      version_byte = 0x04;
      break;
    case BqrVersion::kV7:
      version_byte = 0x05;
      break;
    default:
      break;
  }

  return HalPacket({0x04, 0x0e,         0x16, 0x01, 0x53, 0xfd, 0x00,
                    0x10, 0x01,         0x00, 0x04, 0x00, 0x01, 0x40,
                    0x01, version_byte, 0x01, 0x14, 0x00, 0x01, 0x01,
                    0x07, 0x00,         0x00, 0x00});
}

BqrEvent CreateRootInflammationEvent() {
  return BqrEvent(HalPacket({
      0x04,  // H4 Type: HCI Event
      0xff,  // Event Code: Vendor Specific Event (0xFF)
      0x07,  // Parameter Total Length
      0x58,  // Sub Event: BQR event (0x58)
      0x05,  // Report ID: Root inflammation (0x05)
      0xAA,  // Error Code
      0xBB,  // Vendor Error Code
      0x01,  // Vendor parameters
      0x02,
      0x03,
  }));
}

BqrEvent CreateLinkQualityEvent() {
  std::vector<uint8_t> data = {
      0x04,  // H4 Type: HCI Event
      0xff,  // Event Code: Vendor Specific Event (0xFF)
      0x4e,  // Parameter Total Length
      0x58,  // Sub Event: BQR event (0x58)
      0x02,  // Report ID: A2DP Audio Choppy (0x02)
  };
  data.resize(5 + 78, 0);  // Resize to be a valid V7 packet
  return BqrEvent(HalPacket(data));
}

BqrEvent CreateAdvancedRfStatsEvent() {
  std::vector<uint8_t> data = {
      0x04,  // H4 Type: HCI Event
      0xff,  // Event Code: Vendor Specific Event (0xFF)
      111,   // Parameter Total Length
      0x58,  // Sub Event: BQR event (0x58)
      0x09,  // Report ID: Advance RF Stats (0x09)
  };
  data.resize(5 + 106, 0);  // Resize to be a valid V7 packet
  return BqrEvent(HalPacket(data));
}

BqrEvent CreateEnergyMonitoringEvent() {
  std::vector<uint8_t> data = {
      0x04,  // H4 Type: HCI Event
      0xff,  // Event Code: Vendor Specific Event (0xFF)
      0x59,  // Parameter Total Length
      0x58,  // Sub Event: BQR event (0x58)
      0x06,  // Report ID: Energy Monitoring (0x06)
  };
  data.resize(5 + 84, 0);  // Resize to be a valid V7 packet
  return BqrEvent(HalPacket(data));
}

BqrEvent CreateUnknownBqrEvent() {
  return BqrEvent(HalPacket({
      0x04, 0xff, 0x02, 0x58,
      0xFF,  // Unknown Report ID
  }));
}

// This TestBqrHandler is used to check if monitors are registered correctly.
class TestBqrHandler : public BqrHandler {
 public:
  MOCK_METHOD(void, HandleRootInflammationEvent, (const BqrEvent& event),
              (override));
  MOCK_METHOD(void, HandleLinkQualityEvent, (const BqrEvent& bqr_event),
              (override));
  MOCK_METHOD(void, HandleAdvancedRfStatEvent, (const BqrEvent& bqr_event),
              (override));
  MOCK_METHOD(void, HandleEnergyMonitoringEvent, (const BqrEvent& bqr_event),
              (override));
  MOCK_METHOD(void, HandleUnspecifiedVendorEvent, (const BqrEvent& bqr_event),
              (override));

  BqrVersion GetLocalSupportedBqrVersion() {
    return BqrHandler::GetLocalSupportedBqrVersion();
  }
};

class BqrHandlerTest : public TestWithParam<BqrVersion> {
 protected:
  void SetUp() override {
    mock_debug_central_ = std::make_unique<MockDebugCentral>();
    MockDebugCentral::SetMockDebugCentral(mock_debug_central_.get());

    MockHciRouterClientAgent::SetMockAgent(&mock_hci_router_client_agent_);
    EXPECT_CALL(mock_hci_router_client_agent_, RegisterClient(NotNull()))
        .WillOnce(DoAll(SaveArg<0>(&test_bqr_handler_callack_), Return(true)));
    ON_CALL(mock_hci_router_client_agent_, IsBluetoothChipReady())
        .WillByDefault(Return(false));
    ON_CALL(mock_hci_router_client_agent_, IsBluetoothEnabled())
        .WillByDefault(Return(false));

    // Reset factory between tests
    BqrHandler::RegisterBqrHandler([this]() {
      auto handler = std::make_unique<TestBqrHandler>();
      test_bqr_handler_ = handler.get();
      return handler;
    });
    BqrHandler::Start();

    EXPECT_TRUE(test_bqr_handler_callack_ != nullptr);
    test_bqr_handler_callack_->OnBluetoothEnabled();
  }

  void TearDown() override {
    EXPECT_CALL(mock_hci_router_client_agent_, UnregisterClient(NotNull()))
        .WillOnce(Return(true));

    test_bqr_handler_ = nullptr;
    EXPECT_TRUE(test_bqr_handler_callack_ != nullptr);
    test_bqr_handler_callack_->OnBluetoothDisabled();
    BqrHandler::Stop();
    BqrHandler::RegisterBqrHandler(nullptr);
  }

  std::unique_ptr<MockDebugCentral> mock_debug_central_;
  MockHciRouterClientAgent mock_hci_router_client_agent_;
  HciRouterClientCallback* test_bqr_handler_callack_ = nullptr;
  TestBqrHandler* test_bqr_handler_ = nullptr;
};

TEST_P(BqrHandlerTest, HandleVendorCapabilityEvent) {
  BqrVersion version = GetParam();
  test_bqr_handler_callack_->OnPacketCallback(
      CreateVendorCapabilityEvent(version));
  EXPECT_EQ(test_bqr_handler_->GetLocalSupportedBqrVersion(), version);
}

INSTANTIATE_TEST_SUITE_P(BqrHandlerTest, BqrHandlerTest,
                         Values(BqrVersion::kV1ToV3, BqrVersion::kV4,
                                BqrVersion::kV5, BqrVersion::kV6,
                                BqrVersion::kV7));

TEST_F(BqrHandlerTest, HandleRootInflammationEvent) {
  auto event = CreateRootInflammationEvent();
  EXPECT_CALL(*test_bqr_handler_, HandleRootInflammationEvent(event)).Times(1);

  // Events before setting BQR version are ignored by the handler.
  test_bqr_handler_callack_->OnPacketCallback(event);

  // Events after setting BQR version are processed by the handler.
  test_bqr_handler_callack_->OnPacketCallback(
      CreateVendorCapabilityEvent(BqrVersion::kV7));
  test_bqr_handler_callack_->OnPacketCallback(event);
}

TEST_F(BqrHandlerTest, HandleLinkQualityEvent) {
  auto event = CreateLinkQualityEvent();
  EXPECT_CALL(*test_bqr_handler_, HandleLinkQualityEvent(event)).Times(1);

  // Events before setting BQR version are ignored by the handler.
  test_bqr_handler_callack_->OnPacketCallback(event);

  // Events after setting BQR version are processed by the handler.
  test_bqr_handler_callack_->OnPacketCallback(
      CreateVendorCapabilityEvent(BqrVersion::kV7));
  test_bqr_handler_callack_->OnPacketCallback(event);
}

TEST_F(BqrHandlerTest, HandleAdvancedRfStatEvent) {
  auto event = CreateAdvancedRfStatsEvent();
  EXPECT_CALL(*test_bqr_handler_, HandleAdvancedRfStatEvent(event)).Times(1);

  // Events before setting BQR version are ignored by the handler.
  test_bqr_handler_callack_->OnPacketCallback(event);

  // Events after setting BQR version are processed by the handler.
  test_bqr_handler_callack_->OnPacketCallback(
      CreateVendorCapabilityEvent(BqrVersion::kV7));
  test_bqr_handler_callack_->OnPacketCallback(event);
}

TEST_F(BqrHandlerTest, HandleEnergyMonitoringEvent) {
  auto event = CreateEnergyMonitoringEvent();
  EXPECT_CALL(*test_bqr_handler_, HandleEnergyMonitoringEvent(event)).Times(1);

  // Events before setting BQR version are ignored by the handler.
  test_bqr_handler_callack_->OnPacketCallback(event);

  // Events after setting BQR version are processed by the handler.
  test_bqr_handler_callack_->OnPacketCallback(
      CreateVendorCapabilityEvent(BqrVersion::kV7));
  test_bqr_handler_callack_->OnPacketCallback(event);
}

TEST_F(BqrHandlerTest, HandleUnspecifiedVendorEvent) {
  auto event = CreateUnknownBqrEvent();
  EXPECT_CALL(*test_bqr_handler_, HandleUnspecifiedVendorEvent(event)).Times(1);

  // Events before setting BQR version are ignored by the handler.
  test_bqr_handler_callack_->OnPacketCallback(event);

  // Events after setting BQR version are processed by the handler.
  test_bqr_handler_callack_->OnPacketCallback(
      CreateVendorCapabilityEvent(BqrVersion::kV7));
  test_bqr_handler_callack_->OnPacketCallback(event);
}

}  // namespace
}  // namespace bluetooth_hal::bqr
