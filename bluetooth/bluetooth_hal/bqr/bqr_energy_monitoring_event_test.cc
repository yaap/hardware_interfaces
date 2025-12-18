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

#include "bluetooth_hal/bqr/bqr_energy_monitoring_event.h"

#include <cstdint>
#include <vector>

#include "bluetooth_hal/bqr/bqr_energy_monitoring_event_v6.h"
#include "bluetooth_hal/bqr/bqr_energy_monitoring_event_v7.h"
#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/hal_packet.h"
#include "gtest/gtest.h"

namespace bluetooth_hal::bqr {
namespace {

using ::bluetooth_hal::hci::HalPacket;

HalPacket CreateEnergyMonitoringEvent() {
  std::vector<uint8_t> data = {
      0x04,  // H4 Type: HCI Event
      0xff,  // Event Code: Vendor Specific Event (0xFF)
      0x35,  // Parameter Total Length
      0x58,  // Sub Event: BQR event (0x58)
      0x06,  // Report ID: Energy Monitoring (0x06)

      // Payload
      0x64, 0x00,              // Average Current Consumption (100)
      0xE8, 0x03, 0x00, 0x00,  // Idle Total Time (1000)
      0x0A, 0x00, 0x00, 0x00,  // Idle State Enter Count (10)
      0xF4, 0x01, 0x00, 0x00,  // Active Total Time (500)
      0x05, 0x00, 0x00, 0x00,  // Active State Enter Count (5)
      0xC8, 0x00, 0x00, 0x00,  // BR/EDR Tx Total Time (200)
      0x02, 0x00, 0x00, 0x00,  // BR/EDR Tx State Enter Count (2)
      0xFB,                    // BR/EDR Tx Average Power Level (-5)
      0x90, 0x01, 0x00, 0x00,  // BR/EDR Rx Total Time (400)
      0x04, 0x00, 0x00, 0x00,  // BR/EDR Rx State Enter Count (4)
      0x2C, 0x01, 0x00, 0x00,  // LE Tx Total Time (300)
      0x03, 0x00, 0x00, 0x00,  // LE Tx State Enter Count (3)
      0xFC,                    // LE Tx Average Power Level (-4)
      0x58, 0x02, 0x00, 0x00,  // LE Rx Total Time (600)
      0x06, 0x00, 0x00, 0x00,  // LE Rx State Enter Count (6)
  };
  return HalPacket(data);
}

HalPacket CreateEnergyMonitoringEventV6() {
  std::vector<uint8_t> data = {
      0x04,  // H4 Type: HCI Event
      0xff,  // Event Code: Vendor Specific Event (0xFF)
      0x51,  // Parameter Total Length
      0x58,  // Sub Event: BQR event (0x58)
      0x06,  // Report ID: Energy Monitoring (0x06)

      // Base Payload
      0x64, 0x00, 0xE8, 0x03, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0xF4, 0x01,
      0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x00, 0x00, 0x02, 0x00,
      0x00, 0x00, 0xFB, 0x90, 0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x2C,
      0x01, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xFC, 0x58, 0x02, 0x00, 0x00,
      0x06, 0x00, 0x00, 0x00,

      // V6 Payload
      0x10, 0x27, 0x00, 0x00,  // Report Time Duration (10000)
      0x88, 0x13, 0x00, 0x00,  // RX Active One Chain Time (5000)
      0x50, 0x04, 0x00, 0x00,  // RX Active Two Chain Time (1104)
      0xA0, 0x0F, 0x00, 0x00,  // TX iPA Active One Chain Time (4000)
      0x90, 0x01, 0x00, 0x00,  // TX iPA Active Two Chain Time (400)
      0xD0, 0x07, 0x00, 0x00,  // TX xPA Active One Chain Time (2000)
      0xC8, 0x00, 0x00, 0x00,  // TX xPA Active Two Chain Time (200)
  };
  return HalPacket(data);
}

HalPacket CreateEnergyMonitoringEventV7() {
  std::vector<uint8_t> data = {
      0x04,  // H4 Type: HCI Event
      0xff,  // Event Code: Vendor Specific Event (0xFF)
      0x59,  // Parameter Total Length
      0x58,  // Sub Event: BQR event (0x58)
      0x06,  // Report ID: Energy Monitoring (0x06)

      // Base Payload
      0x64, 0x00, 0xE8, 0x03, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0xF4, 0x01,
      0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x00, 0x00, 0x02, 0x00,
      0x00, 0x00, 0xFB, 0x90, 0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x2C,
      0x01, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xFC, 0x58, 0x02, 0x00, 0x00,
      0x06, 0x00, 0x00, 0x00,

      // V6 Payload
      0x10, 0x27, 0x00, 0x00, 0x88, 0x13, 0x00, 0x00, 0x50, 0x04, 0x00, 0x00,
      0xA0, 0x0F, 0x00, 0x00, 0x90, 0x01, 0x00, 0x00, 0xD0, 0x07, 0x00, 0x00,
      0xC8, 0x00, 0x00, 0x00,

      // V7 Payload
      0x78, 0x02, 0x00,
      0x00,  // BR/EDR Scan RX Active Time (632)
      0x20, 0x03, 0x00,
      0x00,  // LE Scan RX Active Time (800)
  };
  return HalPacket(data);
}

HalPacket CreateIncorrectBqrHalPacket() {
  return HalPacket({0x01, 0x02, 0x03, 0x04, 0x05});
}

HalPacket CreateShortBqrPacket() {
  return HalPacket({
      0x04, 0xff, 0x03, 0x58,
      0x06,  // Report ID
      0x01,  // Payload (but packet is too short)
  });
}

HalPacket CreateWrongReportIdPacket() {
  std::vector<uint8_t> data = {
      0x04, 0xff, 0x35, 0x58,
      0x05,  // Report ID: kRootInflammation (0x05)
  };
  data.resize(58, 0x00);
  return HalPacket(data);
}

void VerifyDefaultValues(const BqrEnergyMonitoringEvent& packet) {
  ASSERT_FALSE(packet.IsValid());
  ASSERT_EQ(packet.GetAverageCurrentConsumption(), 0);
  ASSERT_EQ(packet.GetIdleTotalTime(), 0);
  ASSERT_EQ(packet.GetIdleStateEnterCount(), 0);
  ASSERT_EQ(packet.GetActiveTotalTime(), 0);
  ASSERT_EQ(packet.GetActiveStateEnterCount(), 0);
  ASSERT_EQ(packet.GetBrEdrTxTotalTime(), 0);
  ASSERT_EQ(packet.GetBrEdrTxStateEnterCount(), 0);
  ASSERT_EQ(packet.GetBrEdrTxAveragePowerLevel(), 0);
  ASSERT_EQ(packet.GetBrEdrRxTotalTime(), 0);
  ASSERT_EQ(packet.GetBrEdrRxStateEnterCount(), 0);
  ASSERT_EQ(packet.GetLeTxTotalTime(), 0);
  ASSERT_EQ(packet.GetLeTxStateEnterCount(), 0);
  ASSERT_EQ(packet.GetLeTxAveragePowerLevel(), 0);
  ASSERT_EQ(packet.GetLeRxTotalTime(), 0);
  ASSERT_EQ(packet.GetLeRxStateEnterCount(), 0);
}

void VerifyDefaultValuesV6(const BqrEnergyMonitoringEventV6& packet) {
  VerifyDefaultValues(packet);
  ASSERT_FALSE(packet.IsValid());
  ASSERT_EQ(packet.GetReportTimeDuration(), 0);
  ASSERT_EQ(packet.GetRxActiveOneChainTime(), 0);
  ASSERT_EQ(packet.GetRxActiveTwoChainTime(), 0);
  ASSERT_EQ(packet.GetTxIpaActiveOneChainTime(), 0);
  ASSERT_EQ(packet.GetTxIpaActiveTwoChainTime(), 0);
  ASSERT_EQ(packet.GetTxXpaActiveOneChainTime(), 0);
  ASSERT_EQ(packet.GetTxXpaActiveTwoChainTime(), 0);
}

void VerifyDefaultValuesV7(const BqrEnergyMonitoringEventV7& packet) {
  VerifyDefaultValuesV6(packet);
  ASSERT_FALSE(packet.IsValid());
  ASSERT_EQ(packet.GetBredrRxActiveScanTotaltime(), 0);
  ASSERT_EQ(packet.GetLeRxActiveScanTotaltime(), 0);
}

TEST(BqrEnergyMonitoringEventTest, ValidPacketParsing) {
  auto packet = BqrEnergyMonitoringEvent(CreateEnergyMonitoringEvent());
  ASSERT_TRUE(packet.IsValid());
  ASSERT_EQ(packet.GetBqrReportId(), BqrReportId::kEnergyMonitoring);
  ASSERT_EQ(packet.GetBqrEventType(), BqrEventType::kEnergyMonitoring);

  ASSERT_EQ(packet.GetAverageCurrentConsumption(), 100);
  ASSERT_EQ(packet.GetIdleTotalTime(), 1000);
  ASSERT_EQ(packet.GetIdleStateEnterCount(), 10);
  ASSERT_EQ(packet.GetActiveTotalTime(), 500);
  ASSERT_EQ(packet.GetActiveStateEnterCount(), 5);
  ASSERT_EQ(packet.GetBrEdrTxTotalTime(), 200);
  ASSERT_EQ(packet.GetBrEdrTxStateEnterCount(), 2);
  ASSERT_EQ(packet.GetBrEdrTxAveragePowerLevel(), -5);
  ASSERT_EQ(packet.GetBrEdrRxTotalTime(), 400);
  ASSERT_EQ(packet.GetBrEdrRxStateEnterCount(), 4);
  ASSERT_EQ(packet.GetLeTxTotalTime(), 300);
  ASSERT_EQ(packet.GetLeTxStateEnterCount(), 3);
  ASSERT_EQ(packet.GetLeTxAveragePowerLevel(), -4);
  ASSERT_EQ(packet.GetLeRxTotalTime(), 600);
  ASSERT_EQ(packet.GetLeRxStateEnterCount(), 6);
}

TEST(BqrEnergyMonitoringEventTest, ValidV6PacketParsing) {
  auto packet = BqrEnergyMonitoringEventV6(CreateEnergyMonitoringEventV6());
  ASSERT_TRUE(packet.IsValid());
  ASSERT_EQ(packet.GetBqrReportId(), BqrReportId::kEnergyMonitoring);
  ASSERT_EQ(packet.GetBqrEventType(), BqrEventType::kEnergyMonitoring);

  // Base fields
  ASSERT_EQ(packet.GetAverageCurrentConsumption(), 100);
  ASSERT_EQ(packet.GetIdleTotalTime(), 1000);
  ASSERT_EQ(packet.GetLeRxStateEnterCount(), 6);

  // V6 fields
  ASSERT_EQ(packet.GetReportTimeDuration(), 10000);
  ASSERT_EQ(packet.GetRxActiveOneChainTime(), 5000);
  ASSERT_EQ(packet.GetRxActiveTwoChainTime(), 1104);
  ASSERT_EQ(packet.GetTxIpaActiveOneChainTime(), 4000);
  ASSERT_EQ(packet.GetTxIpaActiveTwoChainTime(), 400);
  ASSERT_EQ(packet.GetTxXpaActiveOneChainTime(), 2000);
  ASSERT_EQ(packet.GetTxXpaActiveTwoChainTime(), 200);
}

TEST(BqrEnergyMonitoringEventTest, ValidV7PacketParsing) {
  auto packet = BqrEnergyMonitoringEventV7(CreateEnergyMonitoringEventV7());
  ASSERT_TRUE(packet.IsValid());
  ASSERT_EQ(packet.GetBqrReportId(), BqrReportId::kEnergyMonitoring);
  ASSERT_EQ(packet.GetBqrEventType(), BqrEventType::kEnergyMonitoring);

  // Base fields
  ASSERT_EQ(packet.GetAverageCurrentConsumption(), 100);
  ASSERT_EQ(packet.GetIdleTotalTime(), 1000);
  ASSERT_EQ(packet.GetLeRxStateEnterCount(), 6);

  // V6 fields
  ASSERT_EQ(packet.GetReportTimeDuration(), 10000);
  ASSERT_EQ(packet.GetRxActiveOneChainTime(), 5000);
  ASSERT_EQ(packet.GetTxXpaActiveTwoChainTime(), 200);

  // V7 fields
  ASSERT_EQ(packet.GetBredrRxActiveScanTotaltime(), 632);
  ASSERT_EQ(packet.GetLeRxActiveScanTotaltime(), 800);
}

TEST(BqrEnergyMonitoringEventTest, InvalidPacketParsingIncorrectFormat) {
  auto packet = BqrEnergyMonitoringEvent(CreateIncorrectBqrHalPacket());
  VerifyDefaultValues(packet);
  auto packet_v6 = BqrEnergyMonitoringEventV6(CreateIncorrectBqrHalPacket());
  VerifyDefaultValuesV6(packet_v6);
  auto packet_v7 = BqrEnergyMonitoringEventV7(CreateIncorrectBqrHalPacket());
  VerifyDefaultValuesV7(packet_v7);
}

TEST(BqrEnergyMonitoringEventTest, InvalidPacketParsingPacketTooShort) {
  auto packet = BqrEnergyMonitoringEvent(CreateShortBqrPacket());
  VerifyDefaultValues(packet);
  auto packet_v6 = BqrEnergyMonitoringEventV6(CreateShortBqrPacket());
  VerifyDefaultValuesV6(packet_v6);
  auto packet_v7 = BqrEnergyMonitoringEventV7(CreateShortBqrPacket());
  VerifyDefaultValuesV7(packet_v7);
}

TEST(BqrEnergyMonitoringEventTest, InvalidPacketParsingWrongReportId) {
  auto packet = BqrEnergyMonitoringEvent(CreateWrongReportIdPacket());
  VerifyDefaultValues(packet);
  auto packet_v6 = BqrEnergyMonitoringEventV6(CreateWrongReportIdPacket());
  VerifyDefaultValuesV6(packet_v6);
  auto packet_v7 = BqrEnergyMonitoringEventV7(CreateWrongReportIdPacket());
  VerifyDefaultValuesV7(packet_v7);
}

}  // namespace
}  // namespace bluetooth_hal::bqr
