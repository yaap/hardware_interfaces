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

#include "bluetooth_hal/bqr/bqr_advance_rf_stats_event.h"

#include <cstdint>
#include <vector>

#include "bluetooth_hal/bqr/bqr_advance_rf_stats_event_v7.h"
#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/hal_packet.h"
#include "gtest/gtest.h"

namespace bluetooth_hal::bqr {
namespace {

using ::bluetooth_hal::hci::HalPacket;

HalPacket CreateAdvanceRfStatsEvent() {
  std::vector<uint8_t> data = {
      0x04,  // H4 Type: HCI Event
      0xff,  // Event Code: Vendor Specific Event (0xFF)
      83,    // Parameter Total Length
      0x58,  // Sub Event: BQR event (0x58)
      0x09,  // Report ID: Advance RF Stats (0x09)

      // Payload
      0x01,                    // ext_info
      0xE8, 0x03, 0x00, 0x00,  // tm_period (1000)
      0x0A, 0x00, 0x00, 0x00,  // tx_pw_ipa_bf
      0x0B, 0x00, 0x00, 0x00,  // tx_pw_epa_bf
      0x0C, 0x00, 0x00, 0x00,  // tx_pw_ipa_div
      0x0D, 0x00, 0x00, 0x00,  // tx_pw_epa_div
      0x14, 0x00, 0x00, 0x00,  // rssi_ch_50
      0x15, 0x00, 0x00, 0x00,  // rssi_ch_50_55
      0x16, 0x00, 0x00, 0x00,  // rssi_ch_55_60
      0x17, 0x00, 0x00, 0x00,  // rssi_ch_60_65
      0x18, 0x00, 0x00, 0x00,  // rssi_ch_65_70
      0x19, 0x00, 0x00, 0x00,  // rssi_ch_70_75
      0x1A, 0x00, 0x00, 0x00,  // rssi_ch_75_80
      0x1B, 0x00, 0x00, 0x00,  // rssi_ch_80_85
      0x1C, 0x00, 0x00, 0x00,  // rssi_ch_85_90
      0x1D, 0x00, 0x00, 0x00,  // rssi_ch_90
      0x28, 0x00, 0x00, 0x00,  // rssi_delta_2_down
      0x29, 0x00, 0x00, 0x00,  // rssi_delta_2_5
      0x2A, 0x00, 0x00, 0x00,  // rssi_delta_5_8
      0x2B, 0x00, 0x00, 0x00,  // rssi_delta_8_11
      0x2C, 0x00, 0x00, 0x00,  // rssi_delta_11_up
  };
  return HalPacket(data);
}

HalPacket CreateAdvanceRfStatsEventV7() {
  std::vector<uint8_t> data = {
      0x04,  // H4 Type: HCI Event
      0xff,  // Event Code: Vendor Specific Event (0xFF)
      111,   // Parameter Total Length
      0x58,  // Sub Event: BQR event (0x58)
      0x09,  // Report ID: Advance RF Stats (0x09)

      // Base Payload
      0x01, 0xE8, 0x03, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00,
      0x00, 0x0C, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00,
      0x00, 0x15, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00,
      0x00, 0x18, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00,
      0x00, 0x1B, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x1D, 0x00, 0x00,
      0x00, 0x28, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x00,
      0x00, 0x2B, 0x00, 0x00, 0x00, 0x2C, 0x00, 0x00, 0x00,

      // V7 Payload
      0x32, 0x00, 0x00, 0x00,  // ant_switch_count
      0x33, 0x00, 0x00, 0x00,  // re_tx_ipa_bf
      0x34, 0x00, 0x00, 0x00,  // re_tx_epa_bf
      0x35, 0x00, 0x00, 0x00,  // re_tx_ipa_div
      0x36, 0x00, 0x00, 0x00,  // re_tx_epa_div
      0x05,                    // ch_count_good
      0x06,                    // ch_count_ok
      0x07,                    // ch_count_bad
      0x08,                    // ch_count_verybad
      0x37, 0x00, 0x00, 0x00,  // tx_buf_queue_count
  };
  return HalPacket(data);
}

HalPacket CreateIncorrectBqrHalPacket() {
  return HalPacket({0x01, 0x02, 0x03, 0x04, 0x05});
}

HalPacket CreateShortBqrPacket() {
  return HalPacket({
      0x04, 0xff, 0x03, 0x58,
      0x09,  // Report ID
      0x01,  // Payload (but packet is too short)
  });
}

HalPacket CreateWrongReportIdPacket() {
  std::vector<uint8_t> data = {
      0x04, 0xff, 83, 0x58,
      0x05,  // Report ID: kRootInflammation (0x05)
  };
  data.resize(111, 0x00);
  return HalPacket(data);
}

void VerifyDefaultValues(const BqrAdvanceRfStatsEvent& packet) {
  ASSERT_FALSE(packet.IsValid());
  ASSERT_EQ(packet.GetExtInfo(), 0);
  ASSERT_EQ(packet.GetTmPeriod(), 0);
  ASSERT_EQ(packet.GetTxPwIpaBf(), 0);
  ASSERT_EQ(packet.GetTxPwEpaBf(), 0);
  ASSERT_EQ(packet.GetTxPwIpaDiv(), 0);
  ASSERT_EQ(packet.GetTxPwEpaDiv(), 0);
  ASSERT_EQ(packet.GetRssiCh50(), 0);
  ASSERT_EQ(packet.GetRssiCh50_55(), 0);
  ASSERT_EQ(packet.GetRssiCh55_60(), 0);
  ASSERT_EQ(packet.GetRssiCh60_65(), 0);
  ASSERT_EQ(packet.GetRssiCh65_70(), 0);
  ASSERT_EQ(packet.GetRssiCh70_75(), 0);
  ASSERT_EQ(packet.GetRssiCh75_80(), 0);
  ASSERT_EQ(packet.GetRssiCh80_85(), 0);
  ASSERT_EQ(packet.GetRssiCh85_90(), 0);
  ASSERT_EQ(packet.GetRssiCh90(), 0);
  ASSERT_EQ(packet.GetRssiDelta2Down(), 0);
  ASSERT_EQ(packet.GetRssiDelta2_5(), 0);
  ASSERT_EQ(packet.GetRssiDelta5_8(), 0);
  ASSERT_EQ(packet.GetRssiDelta8_11(), 0);
  ASSERT_EQ(packet.GetRssiDelta11Up(), 0);
}

void VerifyDefaultValuesV7(const BqrAdvanceRfStatsEventV7& packet) {
  VerifyDefaultValues(packet);
  ASSERT_FALSE(packet.IsValid());
  ASSERT_EQ(packet.GetAntSwitchCount(), 0);
  ASSERT_EQ(packet.GetReTxIpaBf(), 0);
  ASSERT_EQ(packet.GetReTxEpaBf(), 0);
  ASSERT_EQ(packet.GetReTxIpaDiv(), 0);
  ASSERT_EQ(packet.GetReTxEpaDiv(), 0);
  ASSERT_EQ(packet.GetChCountGood(), 0);
  ASSERT_EQ(packet.GetChCountOk(), 0);
  ASSERT_EQ(packet.GetChCountBad(), 0);
  ASSERT_EQ(packet.GetChCountVeryBad(), 0);
  ASSERT_EQ(packet.GetTxBufQueueCount(), 0);
}

TEST(BqrAdvanceRfStatsEventTest, ValidPacketParsing) {
  auto packet = BqrAdvanceRfStatsEvent(CreateAdvanceRfStatsEvent());
  ASSERT_TRUE(packet.IsValid());
  ASSERT_EQ(packet.GetBqrReportId(), BqrReportId::kAdvanceRfStats);
  ASSERT_EQ(packet.GetBqrEventType(), BqrEventType::kAdvancedRfStat);

  ASSERT_EQ(packet.GetExtInfo(), 0x01);
  ASSERT_EQ(packet.GetTmPeriod(), 1000);
  ASSERT_EQ(packet.GetTxPwIpaBf(), 0x0A);
  ASSERT_EQ(packet.GetTxPwEpaBf(), 0x0B);
  ASSERT_EQ(packet.GetTxPwIpaDiv(), 0x0C);
  ASSERT_EQ(packet.GetTxPwEpaDiv(), 0x0D);
  ASSERT_EQ(packet.GetRssiCh50(), 0x14);
  ASSERT_EQ(packet.GetRssiCh50_55(), 0x15);
  ASSERT_EQ(packet.GetRssiCh55_60(), 0x16);
  ASSERT_EQ(packet.GetRssiCh60_65(), 0x17);
  ASSERT_EQ(packet.GetRssiCh65_70(), 0x18);
  ASSERT_EQ(packet.GetRssiCh70_75(), 0x19);
  ASSERT_EQ(packet.GetRssiCh75_80(), 0x1A);
  ASSERT_EQ(packet.GetRssiCh80_85(), 0x1B);
  ASSERT_EQ(packet.GetRssiCh85_90(), 0x1C);
  ASSERT_EQ(packet.GetRssiCh90(), 0x1D);
  ASSERT_EQ(packet.GetRssiDelta2Down(), 0x28);
  ASSERT_EQ(packet.GetRssiDelta2_5(), 0x29);
  ASSERT_EQ(packet.GetRssiDelta5_8(), 0x2A);
  ASSERT_EQ(packet.GetRssiDelta8_11(), 0x2B);
  ASSERT_EQ(packet.GetRssiDelta11Up(), 0x2C);
}

TEST(BqrAdvanceRfStatsEventTest, ValidV7PacketParsing) {
  auto packet = BqrAdvanceRfStatsEventV7(CreateAdvanceRfStatsEventV7());
  ASSERT_TRUE(packet.IsValid());
  ASSERT_EQ(packet.GetBqrReportId(), BqrReportId::kAdvanceRfStats);
  ASSERT_EQ(packet.GetBqrEventType(), BqrEventType::kAdvancedRfStat);

  // Base fields
  ASSERT_EQ(packet.GetExtInfo(), 0x01);
  ASSERT_EQ(packet.GetTmPeriod(), 1000);
  ASSERT_EQ(packet.GetTxPwIpaBf(), 0x0A);

  // V7 fields
  ASSERT_EQ(packet.GetAntSwitchCount(), 50);
  ASSERT_EQ(packet.GetReTxIpaBf(), 51);
  ASSERT_EQ(packet.GetReTxEpaBf(), 52);
  ASSERT_EQ(packet.GetReTxIpaDiv(), 53);
  ASSERT_EQ(packet.GetReTxEpaDiv(), 54);
  ASSERT_EQ(packet.GetChCountGood(), 5);
  ASSERT_EQ(packet.GetChCountOk(), 6);
  ASSERT_EQ(packet.GetChCountBad(), 7);
  ASSERT_EQ(packet.GetChCountVeryBad(), 8);
  ASSERT_EQ(packet.GetTxBufQueueCount(), 55);
}

TEST(BqrAdvanceRfStatsEventTest, InvalidPacketParsingIncorrectFormat) {
  auto packet = BqrAdvanceRfStatsEvent(CreateIncorrectBqrHalPacket());
  VerifyDefaultValues(packet);
  auto packet_v7 = BqrAdvanceRfStatsEventV7(CreateIncorrectBqrHalPacket());
  VerifyDefaultValuesV7(packet_v7);
}

TEST(BqrAdvanceRfStatsEventTest, InvalidPacketParsingPacketTooShort) {
  auto packet = BqrAdvanceRfStatsEvent(CreateShortBqrPacket());
  VerifyDefaultValues(packet);
  auto packet_v7 = BqrAdvanceRfStatsEventV7(CreateShortBqrPacket());
  VerifyDefaultValuesV7(packet_v7);
}

TEST(BqrAdvanceRfStatsEventTest, InvalidPacketParsingWrongReportId) {
  auto packet = BqrAdvanceRfStatsEvent(CreateWrongReportIdPacket());
  VerifyDefaultValues(packet);
  auto packet_v7 = BqrAdvanceRfStatsEventV7(CreateWrongReportIdPacket());
  VerifyDefaultValuesV7(packet_v7);
}

}  // namespace
}  // namespace bluetooth_hal::bqr
