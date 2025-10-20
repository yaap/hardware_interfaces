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

#include "bluetooth_hal/bqr/bqr_energy_monitoring_event.h"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>

#include "bluetooth_hal/bqr/bqr_event.h"
#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace bqr {
namespace {

using ::bluetooth_hal::hci::HalPacket;

constexpr size_t kEnergyMonitoringEventMinSize =
    static_cast<size_t>(EnergyMonitoringOffset::kEnd);

}  // namespace

BqrEnergyMonitoringEvent::BqrEnergyMonitoringEvent(const HalPacket& packet)
    : BqrEvent(packet),
      is_valid_(BqrEvent::IsValid() &&
                GetBqrEventType() == BqrEventType::kEnergyMonitoring &&
                size() >= kEnergyMonitoringEventMinSize) {
  ParseData();
}

void BqrEnergyMonitoringEvent::ParseData() {
  if (is_valid_) {
    average_current_consumption_ = AtUint16LittleEndian(
        EnergyMonitoringOffset::kAverageCurrentConsumption);
    idle_total_time_ =
        AtUint32LittleEndian(EnergyMonitoringOffset::kIdleTotalTime);
    idle_state_enter_count_ =
        AtUint32LittleEndian(EnergyMonitoringOffset::kIdleStateEnterCount);
    active_total_time_ =
        AtUint32LittleEndian(EnergyMonitoringOffset::kActiveTotalTime);
    active_state_enter_count_ =
        AtUint32LittleEndian(EnergyMonitoringOffset::kActiveStateEnterCount);
    br_edr_tx_total_time_ =
        AtUint32LittleEndian(EnergyMonitoringOffset::kBrEdrTxTotalTime);
    br_edr_tx_state_enter_count_ =
        AtUint32LittleEndian(EnergyMonitoringOffset::kBrEdrTxStateEnterCount);
    br_edr_tx_average_power_level_ = static_cast<int8_t>(
        At(EnergyMonitoringOffset::kBrEdrTxAveragePowerLevel));
    br_edr_rx_total_time_ =
        AtUint32LittleEndian(EnergyMonitoringOffset::kBrEdrRxTotalTime);
    br_edr_rx_state_enter_count_ =
        AtUint32LittleEndian(EnergyMonitoringOffset::kBrEdrRxStateEnterCount);
    le_tx_total_time_ =
        AtUint32LittleEndian(EnergyMonitoringOffset::kLeTxTotalTime);
    le_tx_state_enter_count_ =
        AtUint32LittleEndian(EnergyMonitoringOffset::kLeTxStateEnterCount);
    le_tx_average_power_level_ =
        static_cast<int8_t>(At(EnergyMonitoringOffset::kLeTxAveragePowerLevel));
    le_rx_total_time_ =
        AtUint32LittleEndian(EnergyMonitoringOffset::kLeRxTotalTime);
    le_rx_state_enter_count_ =
        AtUint32LittleEndian(EnergyMonitoringOffset::kLeRxStateEnterCount);
  }
}

bool BqrEnergyMonitoringEvent::IsValid() const { return is_valid_; }

uint16_t BqrEnergyMonitoringEvent::GetAverageCurrentConsumption() const {
  return average_current_consumption_;
}

uint32_t BqrEnergyMonitoringEvent::GetIdleTotalTime() const {
  return idle_total_time_;
}

uint32_t BqrEnergyMonitoringEvent::GetIdleStateEnterCount() const {
  return idle_state_enter_count_;
}

uint32_t BqrEnergyMonitoringEvent::GetActiveTotalTime() const {
  return active_total_time_;
}

uint32_t BqrEnergyMonitoringEvent::GetActiveStateEnterCount() const {
  return active_state_enter_count_;
}

uint32_t BqrEnergyMonitoringEvent::GetBrEdrTxTotalTime() const {
  return br_edr_tx_total_time_;
}

uint32_t BqrEnergyMonitoringEvent::GetBrEdrTxStateEnterCount() const {
  return br_edr_tx_state_enter_count_;
}

int8_t BqrEnergyMonitoringEvent::GetBrEdrTxAveragePowerLevel() const {
  return br_edr_tx_average_power_level_;
}

uint32_t BqrEnergyMonitoringEvent::GetBrEdrRxTotalTime() const {
  return br_edr_rx_total_time_;
}

uint32_t BqrEnergyMonitoringEvent::GetBrEdrRxStateEnterCount() const {
  return br_edr_rx_state_enter_count_;
}

uint32_t BqrEnergyMonitoringEvent::GetLeTxTotalTime() const {
  return le_tx_total_time_;
}

uint32_t BqrEnergyMonitoringEvent::GetLeTxStateEnterCount() const {
  return le_tx_state_enter_count_;
}

int8_t BqrEnergyMonitoringEvent::GetLeTxAveragePowerLevel() const {
  return le_tx_average_power_level_;
}

uint32_t BqrEnergyMonitoringEvent::GetLeRxTotalTime() const {
  return le_rx_total_time_;
}

uint32_t BqrEnergyMonitoringEvent::GetLeRxStateEnterCount() const {
  return le_rx_state_enter_count_;
}

std::string BqrEnergyMonitoringEvent::ToString() const {
  if (!is_valid_) {
    return "BqrEnergyMonitoringEvent(Invalid)";
  }
  return "BqrEnergyMonitoringEvent: " + ToBqrString();
}

std::string BqrEnergyMonitoringEvent::ToBqrString() const {
  std::stringstream ss;
  ss << "Avg_Cur_Pwr: " << std::dec << average_current_consumption_
     << " mA, BEr_Tx_Plv: " << static_cast<int>(br_edr_tx_average_power_level_)
     << " dBm, Le_Tx_Plv: " << static_cast<int>(le_tx_average_power_level_)
     << " dBm, Idle_Tm: " << idle_total_time_
     << " ms, Act_Tm: " << active_total_time_
     << " ms, BEr_Tx_Tm: " << br_edr_tx_total_time_
     << " ms, BEr_Rx_Tm: " << br_edr_rx_total_time_
     << " ms, Le_Tx_Tm: " << le_tx_total_time_
     << " ms, Le_Rx_Tm: " << le_rx_total_time_ << " ms";
  return ss.str();
}

}  // namespace bqr
}  // namespace bluetooth_hal
