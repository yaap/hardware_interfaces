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

#include "bluetooth_hal/bqr/bqr_energy_monitoring_event_v7.h"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

#include "bluetooth_hal/bqr/bqr_energy_monitoring_event_v6.h"
#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace bqr {
namespace {

using ::bluetooth_hal::hci::HalPacket;

constexpr size_t kEnergyMonitoringEventV7MinSize =
    static_cast<size_t>(EnergyMonitoringOffsetV7::kEnd);

}  // namespace

BqrEnergyMonitoringEventV7::BqrEnergyMonitoringEventV7(const HalPacket& packet)
    : BqrEnergyMonitoringEventV6(packet),
      bredr_rx_active_scan_totaltime_(0),
      le_rx_active_scan_totaltime_(0) {
  is_valid_ = BqrEnergyMonitoringEventV6::IsValid() &&
              size() >= kEnergyMonitoringEventV7MinSize;
  ParseData();
}

void BqrEnergyMonitoringEventV7::ParseData() {
  if (is_valid_) {
    bredr_rx_active_scan_totaltime_ = AtUint32LittleEndian(
        EnergyMonitoringOffsetV7::kBredrRxActiveScanTotaltime);
    le_rx_active_scan_totaltime_ = AtUint32LittleEndian(
        EnergyMonitoringOffsetV7::kLeRxActiveScanTotaltime);
  }
}

bool BqrEnergyMonitoringEventV7::IsValid() const { return is_valid_; }

uint32_t BqrEnergyMonitoringEventV7::GetBredrRxActiveScanTotaltime() const {
  return bredr_rx_active_scan_totaltime_;
}

uint32_t BqrEnergyMonitoringEventV7::GetLeRxActiveScanTotaltime() const {
  return le_rx_active_scan_totaltime_;
}

std::string BqrEnergyMonitoringEventV7::ToString() const {
  if (!is_valid_) {
    return "BqrEnergyMonitoringEventV7(Invalid)";
  }
  return "BqrEnergyMonitoringEventV7: " + ToBqrString();
}

std::string BqrEnergyMonitoringEventV7::ToBqrString() const {
  std::stringstream ss;
  ss << BqrEnergyMonitoringEventV6::ToBqrString()
     << ", BR/EDR_Rx_Act_Scan: " << std::dec << bredr_rx_active_scan_totaltime_
     << " ms, LE_Rx_Act_Scan: " << le_rx_active_scan_totaltime_ << " ms";
  return ss.str();
}

}  // namespace bqr
}  // namespace bluetooth_hal
