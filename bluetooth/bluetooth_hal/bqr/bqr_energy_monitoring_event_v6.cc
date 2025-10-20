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

#include "bluetooth_hal/bqr/bqr_energy_monitoring_event_v6.h"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

#include "bluetooth_hal/bqr/bqr_energy_monitoring_event.h"
#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace bqr {
namespace {

using ::bluetooth_hal::hci::HalPacket;

constexpr size_t kEnergyMonitoringEventV6MinSize =
    static_cast<size_t>(EnergyMonitoringOffsetV6::kEnd);

}  // namespace

BqrEnergyMonitoringEventV6::BqrEnergyMonitoringEventV6(const HalPacket& packet)
    : BqrEnergyMonitoringEvent(packet) {
  is_valid_ = BqrEnergyMonitoringEvent::IsValid() &&
              size() >= kEnergyMonitoringEventV6MinSize;
  ParseData();
}

void BqrEnergyMonitoringEventV6::ParseData() {
  if (is_valid_) {
    report_time_duration_ =
        AtUint32LittleEndian(EnergyMonitoringOffsetV6::kReportTimeDuration);
    rx_active_one_chain_time_ =
        AtUint32LittleEndian(EnergyMonitoringOffsetV6::kRxActiveOneChainTime);
    rx_active_two_chain_time_ =
        AtUint32LittleEndian(EnergyMonitoringOffsetV6::kRxActiveTwoChainTime);
    tx_ipa_active_one_chain_time_ = AtUint32LittleEndian(
        EnergyMonitoringOffsetV6::kTxIpaActiveOneChainTime);
    tx_ipa_active_two_chain_time_ = AtUint32LittleEndian(
        EnergyMonitoringOffsetV6::kTxIpaActiveTwoChainTime);
    tx_xpa_active_one_chain_time_ = AtUint32LittleEndian(
        EnergyMonitoringOffsetV6::kTxXpaActiveOneChainTime);
    tx_xpa_active_two_chain_time_ = AtUint32LittleEndian(
        EnergyMonitoringOffsetV6::kTxXpaActiveTwoChainTime);
  }
}

bool BqrEnergyMonitoringEventV6::IsValid() const { return is_valid_; }

uint32_t BqrEnergyMonitoringEventV6::GetReportTimeDuration() const {
  return report_time_duration_;
}

uint32_t BqrEnergyMonitoringEventV6::GetRxActiveOneChainTime() const {
  return rx_active_one_chain_time_;
}

uint32_t BqrEnergyMonitoringEventV6::GetRxActiveTwoChainTime() const {
  return rx_active_two_chain_time_;
}

uint32_t BqrEnergyMonitoringEventV6::GetTxIpaActiveOneChainTime() const {
  return tx_ipa_active_one_chain_time_;
}

uint32_t BqrEnergyMonitoringEventV6::GetTxIpaActiveTwoChainTime() const {
  return tx_ipa_active_two_chain_time_;
}

uint32_t BqrEnergyMonitoringEventV6::GetTxXpaActiveOneChainTime() const {
  return tx_xpa_active_one_chain_time_;
}

uint32_t BqrEnergyMonitoringEventV6::GetTxXpaActiveTwoChainTime() const {
  return tx_xpa_active_two_chain_time_;
}

std::string BqrEnergyMonitoringEventV6::ToString() const {
  if (!is_valid_) {
    return "BqrEnergyMonitoringEventV6(Invalid)";
  }
  return "BqrEnergyMonitoringEventV6: " + ToBqrString();
}

std::string BqrEnergyMonitoringEventV6::ToBqrString() const {
  std::stringstream ss;
  ss << BqrEnergyMonitoringEvent::ToBqrString() << ", Rpt_Dur: " << std::dec
     << report_time_duration_
     << " ms, Rx_Act_1_Chain: " << rx_active_one_chain_time_
     << " ms, Rx_Act_2_Chain: " << rx_active_two_chain_time_
     << " ms, Tx_iPA_Act_1_Chain: " << tx_ipa_active_one_chain_time_
     << " ms, Tx_iPA_Act_2_Chain: " << tx_ipa_active_two_chain_time_
     << " ms, Tx_xPA_Act_1_Chain: " << tx_xpa_active_one_chain_time_
     << " ms, Tx_xPA_Act_2_Chain: " << tx_xpa_active_two_chain_time_ << " ms";
  return ss.str();
}

}  // namespace bqr
}  // namespace bluetooth_hal
