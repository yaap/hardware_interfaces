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

#include "bluetooth_hal/bqr/bqr_advance_rf_stats_event_v7.h"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>

#include "bluetooth_hal/bqr/bqr_advance_rf_stats_event.h"
#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace bqr {
namespace {

using ::bluetooth_hal::hci::HalPacket;

constexpr size_t kAdvanceRfStatsEventV7MinSize =
    static_cast<size_t>(AdvanceRfStatsOffsetV7::kEnd);

}  // namespace

BqrAdvanceRfStatsEventV7::BqrAdvanceRfStatsEventV7(const HalPacket& packet)
    : BqrAdvanceRfStatsEvent(packet) {
  is_valid_ = BqrAdvanceRfStatsEvent::IsValid() &&
              size() >= kAdvanceRfStatsEventV7MinSize;
  ParseData();
}

void BqrAdvanceRfStatsEventV7::ParseData() {
  if (is_valid_) {
    ant_switch_count_ =
        AtUint32LittleEndian(AdvanceRfStatsOffsetV7::kAntSwitchCount);
    re_tx_ipa_bf_ = AtUint32LittleEndian(AdvanceRfStatsOffsetV7::kReTxIpaBf);
    re_tx_epa_bf_ = AtUint32LittleEndian(AdvanceRfStatsOffsetV7::kReTxEpaBf);
    re_tx_ipa_div_ = AtUint32LittleEndian(AdvanceRfStatsOffsetV7::kReTxIpaDiv);
    re_tx_epa_div_ = AtUint32LittleEndian(AdvanceRfStatsOffsetV7::kReTxEpaDiv);
    ch_count_good_ = At(AdvanceRfStatsOffsetV7::kChCountGood);
    ch_count_ok_ = At(AdvanceRfStatsOffsetV7::kChCountOk);
    ch_count_bad_ = At(AdvanceRfStatsOffsetV7::kChCountBad);
    ch_count_verybad_ = At(AdvanceRfStatsOffsetV7::kChCountVeryBad);
    tx_buf_queue_count_ =
        AtUint32LittleEndian(AdvanceRfStatsOffsetV7::kTxBufQueueCount);
  }
}

bool BqrAdvanceRfStatsEventV7::IsValid() const { return is_valid_; }

uint32_t BqrAdvanceRfStatsEventV7::GetAntSwitchCount() const {
  return ant_switch_count_;
}

uint32_t BqrAdvanceRfStatsEventV7::GetReTxIpaBf() const {
  return re_tx_ipa_bf_;
}

uint32_t BqrAdvanceRfStatsEventV7::GetReTxEpaBf() const {
  return re_tx_epa_bf_;
}

uint32_t BqrAdvanceRfStatsEventV7::GetReTxIpaDiv() const {
  return re_tx_ipa_div_;
}

uint32_t BqrAdvanceRfStatsEventV7::GetReTxEpaDiv() const {
  return re_tx_epa_div_;
}

uint8_t BqrAdvanceRfStatsEventV7::GetChCountGood() const {
  return ch_count_good_;
}

uint8_t BqrAdvanceRfStatsEventV7::GetChCountOk() const { return ch_count_ok_; }

uint8_t BqrAdvanceRfStatsEventV7::GetChCountBad() const {
  return ch_count_bad_;
}

uint8_t BqrAdvanceRfStatsEventV7::GetChCountVeryBad() const {
  return ch_count_verybad_;
}

uint32_t BqrAdvanceRfStatsEventV7::GetTxBufQueueCount() const {
  return tx_buf_queue_count_;
}

std::string BqrAdvanceRfStatsEventV7::ToString() const {
  if (!is_valid_) {
    return "BqrAdvanceRfStatsEventV7(Invalid)";
  }
  return "BqrAdvanceRfStatsEventV7: " + ToBqrString();
}

std::string BqrAdvanceRfStatsEventV7::ToBqrString() const {
  std::stringstream ss;
  ss << BqrAdvanceRfStatsEvent::ToBqrString()
     << ", Ant_Switch_Count: " << std::dec << ant_switch_count_
     << ", Re_Tx_iPA_BF: " << std::dec << re_tx_ipa_bf_
     << ", Re_Tx_ePA_BF: " << std::dec << re_tx_epa_bf_
     << ", Re_Tx_iPA_Div: " << std::dec << re_tx_ipa_div_
     << ", Re_Tx_ePA_Div: " << std::dec << re_tx_epa_div_
     << ", Ch_Cnt_good: " << std::dec << static_cast<int>(ch_count_good_)
     << ", Ch_Cnt_OK: " << std::dec << static_cast<int>(ch_count_ok_)
     << ", Ch_Cnt_Bad: " << std::dec << static_cast<int>(ch_count_bad_)
     << ", Ch_Cnt_Verybad: " << std::dec << static_cast<int>(ch_count_verybad_)
     << ", TX_Buf_Que_Cnt: " << std::dec << tx_buf_queue_count_;
  return ss.str();
}

}  // namespace bqr
}  // namespace bluetooth_hal
