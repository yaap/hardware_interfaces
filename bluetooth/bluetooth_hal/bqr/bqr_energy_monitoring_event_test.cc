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

#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/hal_packet.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace bqr {
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

TEST(BqrEnergyMonitoringEventTest, InvalidPacketParsingIncorrectFormat) {
  auto packet = BqrEnergyMonitoringEvent(CreateIncorrectBqrHalPacket());
  VerifyDefaultValues(packet);
}

TEST(BqrEnergyMonitoringEventTest, InvalidPacketParsingPacketTooShort) {
  auto packet = BqrEnergyMonitoringEvent(CreateShortBqrPacket());
  VerifyDefaultValues(packet);
}

TEST(BqrEnergyMonitoringEventTest, InvalidPacketParsingWrongReportId) {
  auto packet = BqrEnergyMonitoringEvent(CreateWrongReportIdPacket());
  VerifyDefaultValues(packet);
}

}  // namespace
}  // namespace bqr
}  // namespace bluetooth_hal
