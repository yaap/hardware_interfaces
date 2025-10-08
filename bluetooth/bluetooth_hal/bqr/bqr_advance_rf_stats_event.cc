/*
 * Copyright 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bluetooth_hal/bqr/bqr_advance_rf_stats_event.h"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>

#include "bluetooth_hal/bqr/bqr_event.h"
#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace bqr {
namespace {

using ::bluetooth_hal::hci::HalPacket;

constexpr size_t kAdvanceRfStatsEventMinSize =
    static_cast<size_t>(AdvanceRfStatsOffset::kEnd);

}  // namespace

BqrAdvanceRfStatsEvent::BqrAdvanceRfStatsEvent(const HalPacket& packet)
    : BqrEvent(packet),
      is_valid_(BqrEvent::IsValid() &&
                GetBqrEventType() == BqrEventType::kAdvancedRfStat &&
                size() >= kAdvanceRfStatsEventMinSize) {
  ParseData();
}

void BqrAdvanceRfStatsEvent::ParseData() {
  if (is_valid_) {
    ext_info_ = At(AdvanceRfStatsOffset::kExtInfo);
    tm_period_ = AtUint32LittleEndian(AdvanceRfStatsOffset::kTmPeriod);
    tx_pw_ipa_bf_ = AtUint32LittleEndian(AdvanceRfStatsOffset::kTxPwIpaBf);
    tx_pw_epa_bf_ = AtUint32LittleEndian(AdvanceRfStatsOffset::kTxPwEpaBf);
    tx_pw_ipa_div_ = AtUint32LittleEndian(AdvanceRfStatsOffset::kTxPwIpaDiv);
    tx_pw_epa_div_ = AtUint32LittleEndian(AdvanceRfStatsOffset::kTxPwEpaDiv);
    rssi_ch_50_ = AtUint32LittleEndian(AdvanceRfStatsOffset::kRssiCh50);
    rssi_ch_50_55_ = AtUint32LittleEndian(AdvanceRfStatsOffset::kRssiCh50_55);
    rssi_ch_55_60_ = AtUint32LittleEndian(AdvanceRfStatsOffset::kRssiCh55_60);
    rssi_ch_60_65_ = AtUint32LittleEndian(AdvanceRfStatsOffset::kRssiCh60_65);
    rssi_ch_65_70_ = AtUint32LittleEndian(AdvanceRfStatsOffset::kRssiCh65_70);
    rssi_ch_70_75_ = AtUint32LittleEndian(AdvanceRfStatsOffset::kRssiCh70_75);
    rssi_ch_75_80_ = AtUint32LittleEndian(AdvanceRfStatsOffset::kRssiCh75_80);
    rssi_ch_80_85_ = AtUint32LittleEndian(AdvanceRfStatsOffset::kRssiCh80_85);
    rssi_ch_85_90_ = AtUint32LittleEndian(AdvanceRfStatsOffset::kRssiCh85_90);
    rssi_ch_90_ = AtUint32LittleEndian(AdvanceRfStatsOffset::kRssiCh90);
    rssi_delta_2_down_ =
        AtUint32LittleEndian(AdvanceRfStatsOffset::kRssiDelta2Down);
    rssi_delta_2_5_ = AtUint32LittleEndian(AdvanceRfStatsOffset::kRssiDelta2_5);
    rssi_delta_5_8_ = AtUint32LittleEndian(AdvanceRfStatsOffset::kRssiDelta5_8);
    rssi_delta_8_11_ =
        AtUint32LittleEndian(AdvanceRfStatsOffset::kRssiDelta8_11);
    rssi_delta_11_up_ =
        AtUint32LittleEndian(AdvanceRfStatsOffset::kRssiDelta11Up);
  }
}
bool BqrAdvanceRfStatsEvent::IsValid() const { return is_valid_; }

uint8_t BqrAdvanceRfStatsEvent::GetExtInfo() const { return ext_info_; }

uint32_t BqrAdvanceRfStatsEvent::GetTmPeriod() const { return tm_period_; }

uint32_t BqrAdvanceRfStatsEvent::GetTxPwIpaBf() const { return tx_pw_ipa_bf_; }

uint32_t BqrAdvanceRfStatsEvent::GetTxPwEpaBf() const { return tx_pw_epa_bf_; }

uint32_t BqrAdvanceRfStatsEvent::GetTxPwIpaDiv() const {
  return tx_pw_ipa_div_;
}

uint32_t BqrAdvanceRfStatsEvent::GetTxPwEpaDiv() const {
  return tx_pw_epa_div_;
}

uint32_t BqrAdvanceRfStatsEvent::GetRssiCh50() const { return rssi_ch_50_; }

uint32_t BqrAdvanceRfStatsEvent::GetRssiCh50_55() const {
  return rssi_ch_50_55_;
}

uint32_t BqrAdvanceRfStatsEvent::GetRssiCh55_60() const {
  return rssi_ch_55_60_;
}

uint32_t BqrAdvanceRfStatsEvent::GetRssiCh60_65() const {
  return rssi_ch_60_65_;
}

uint32_t BqrAdvanceRfStatsEvent::GetRssiCh65_70() const {
  return rssi_ch_65_70_;
}

uint32_t BqrAdvanceRfStatsEvent::GetRssiCh70_75() const {
  return rssi_ch_70_75_;
}

uint32_t BqrAdvanceRfStatsEvent::GetRssiCh75_80() const {
  return rssi_ch_75_80_;
}

uint32_t BqrAdvanceRfStatsEvent::GetRssiCh80_85() const {
  return rssi_ch_80_85_;
}

uint32_t BqrAdvanceRfStatsEvent::GetRssiCh85_90() const {
  return rssi_ch_85_90_;
}

uint32_t BqrAdvanceRfStatsEvent::GetRssiCh90() const { return rssi_ch_90_; }

uint32_t BqrAdvanceRfStatsEvent::GetRssiDelta2Down() const {
  return rssi_delta_2_down_;
}

uint32_t BqrAdvanceRfStatsEvent::GetRssiDelta2_5() const {
  return rssi_delta_2_5_;
}

uint32_t BqrAdvanceRfStatsEvent::GetRssiDelta5_8() const {
  return rssi_delta_5_8_;
}

uint32_t BqrAdvanceRfStatsEvent::GetRssiDelta8_11() const {
  return rssi_delta_8_11_;
}

uint32_t BqrAdvanceRfStatsEvent::GetRssiDelta11Up() const {
  return rssi_delta_11_up_;
}

std::string BqrAdvanceRfStatsEvent::ToString() const {
  if (!is_valid_) {
    return "BqrAdvanceRfStatsEvent(Invalid)";
  }
  return "BqrAdvanceRfStatsEvent: " + ToBqrString();
}

std::string BqrAdvanceRfStatsEvent::ToBqrString() const {
  std::stringstream ss;
  ss << "Time Period:" << std::dec << tm_period_ << " ms"
     << ", Extension id: " << std::dec << static_cast<int>(ext_info_)
     << ", TW_Pw_iPA_BF: " << std::dec << tx_pw_ipa_bf_
     << ", TW_Pw_ePA_BF: " << std::dec << tx_pw_epa_bf_
     << ", TW_Pw_iPA_Div: " << std::dec << tx_pw_ipa_div_
     << ", TW_Pw_ePA_Div: " << std::dec << tx_pw_epa_div_
     << ", RSSI_Chain_>-50: " << std::dec << rssi_ch_50_
     << ", RSSI_Chain_-50_-55: " << std::dec << rssi_ch_50_55_
     << ", RSSI_Chain_-55_-60: " << std::dec << rssi_ch_55_60_
     << ", RSSI_Chain_-60_-65: " << std::dec << rssi_ch_60_65_
     << ", RSSI_Chain_-65_-70: " << std::dec << rssi_ch_65_70_
     << ", RSSI_Chain_-70_-75: " << std::dec << rssi_ch_70_75_
     << ", RSSI_Chain_-75_-80: " << std::dec << rssi_ch_75_80_
     << ", RSSI_Chain_-80_-85: " << std::dec << rssi_ch_80_85_
     << ", RSSI_Chain_-85_-90: " << std::dec << rssi_ch_85_90_
     << ", RSSI_Chain_<-90: " << std::dec << rssi_ch_90_
     << ", RSSI_Delta_<2: " << std::dec << rssi_delta_2_down_
     << ", RSSI_Delta_2_5: " << std::dec << rssi_delta_2_5_
     << ", RSSI_Delta_5_8: " << std::dec << rssi_delta_5_8_
     << ", RSSI_Delta_8_11: " << std::dec << rssi_delta_8_11_
     << ", RSSI_Delta_>11: " << std::dec << rssi_delta_11_up_;
  return ss.str();
}

}  // namespace bqr
}  // namespace bluetooth_hal
