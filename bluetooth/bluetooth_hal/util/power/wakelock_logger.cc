/*
 * Copyright 2026 The Android Open Source Project
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

#define LOG_TAG "bluetooth_hal.wakelock_logger"

#include "bluetooth_hal/util/power/wakelock_logger.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "android-base/logging.h"
#include "bluetooth_hal/util/time_queue.h"

namespace bluetooth_hal::util::power {

using ::bluetooth_hal::debug::Coredump;
using ::bluetooth_hal::debug::CoredumpPosition;
using ::bluetooth_hal::hci::HciPacketType;

static constexpr struct {
    TimeUnit unit;
    int ms;
} kTimeUnitThresholds[] = {
        {TimeUnit::kTwoFiftyMs, 250},   {TimeUnit::kFiveHundredMs, 500},
        {TimeUnit::kOneSec, 1000},      {TimeUnit::kTwoSec, 2000},
        {TimeUnit::kFourSec, 4000},     {TimeUnit::kEightSec, 8000},
        {TimeUnit::kSixteenSec, 16000}, {TimeUnit::kThirtySec, 30000},
        {TimeUnit::kOneMin, 60000},     {TimeUnit::kFiveMin, 300000},
        {TimeUnit::kTenMin, 600000},    {TimeUnit::kTwentyMin, 1200000},
};

static constexpr int kDefaultWeight = 1;
static constexpr int kPromotedWeight = 5;

WakelockLogger::WakelockLogger() : history_() {}

std::vector<Coredump> WakelockLogger::Dump() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::stringstream ss;

    bool first = true;
    for (const auto& slot : history_) {
        if (first) {
            ss << "╔═══════════════════════════════════════════════════════════════════════════════"
                  "══════════════════════════════════\n";
            first = false;
        } else {
            ss << "╠═══════════════════════════════════════════════════════════════════════════════"
                  "══════════════════════════════════\n";
        }
        ss << "║ Timeslot: " << slot.TimePeriodToString() << "\n";
        ss << "╠═══════════════════════════════════════════════════════════════════════════════════"
              "══════════════════════════════\n";
        ss << "║ " << std::left << std::setw(11) << " "
           << "\t250ms\t500ms\t1s\t2s\t4s\t8s\t16s\t30s\t1m\t5m\t10m\t20m\t20m+\n";

        auto print_row =
                [&](const std::string& name,
                    const std::array<int, static_cast<size_t>(TimeUnit::kTimeUnitMax)>& data) {
                    ss << "║ " << std::left << std::setw(11) << name;
                    for (int val : data) {
                        ss << "\t" << val;
                    }
                    ss << "\n";
                };

        print_row("ACL RX", slot.data.acl_rx);

        print_row("ACL TX", slot.data.acl_tx);
        print_row("Command", slot.data.commands);
        print_row("Event", slot.data.events);
    }
    if (!history_.empty()) {
        ss << "╚═══════════════════════════════════════════════════════════════════════════════════"
              "══════════════════════════════\n";
    }

    return {{.title = "Wakelock Logger", .coredump = ss.str(), .position = CoredumpPosition::kEnd}};
}

void WakelockLogger::StartSession() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_session_active_) {
        return;
    }
    session_start_time_ = std::chrono::system_clock::now();
    is_session_active_ = true;
}

void WakelockLogger::RecordActivity(WakeSource source, HciPacketType type) {
    StartSession();

    std::lock_guard<std::mutex> lock(mutex_);
    int weight = kDefaultWeight;
    if (!history_.empty()) {
        // Promote the wake reason of the wakelock session.
        weight = kPromotedWeight;
    }

    if (source == WakeSource::kTx && type == HciPacketType::kAclData) {
        tx_count_ += weight;
    } else if (source == WakeSource::kRx && type == HciPacketType::kAclData) {
        rx_count_ += weight;
    }

    if (type == HciPacketType::kCommand) {
        command_count_ += weight;
    } else if (type == HciPacketType::kEvent) {
        event_count_ += weight;
    }
}

void WakelockLogger::EndSession() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_session_active_) {
        LOG(WARNING) << "EndSession called without an active session.";
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - session_start_time_)
                         .count();

    TimeUnit unit = TimeUnit::kTwentyMinPlus;
    for (const auto& threshold : kTimeUnitThresholds) {
        if (delta <= threshold.ms) {
            unit = threshold.unit;
            break;
        }
    }

    int max_count = std::max({tx_count_, rx_count_, command_count_, event_count_});
    if (max_count > 0) {
        auto& slot = history_.current();
        size_t index = static_cast<size_t>(unit);

        // Categorize the session by the most frequent activity. If there is a tie,
        // use this priority: TX > Command > RX > Event.
        if (max_count == tx_count_) {
            slot.acl_tx[index]++;
        } else if (max_count == command_count_) {
            slot.commands[index]++;
        } else if (max_count == rx_count_) {
            slot.acl_rx[index]++;
        } else if (max_count == event_count_) {
            slot.events[index]++;
        }
    }

    tx_count_ = 0;
    rx_count_ = 0;
    command_count_ = 0;
    event_count_ = 0;
    is_session_active_ = false;
}

}  // namespace bluetooth_hal::util::power
