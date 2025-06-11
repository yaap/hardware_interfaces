/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include "RadioNetwork.h"

#include <libminradio/debug.h>
#include <libminradio/network/structs.h>
#include <libminradio/response.h>

#define RADIO_MODULE "NetworkImpl"

namespace android::hardware::radio::service {

using ::aidl::android::hardware::radio::RadioConst;
using ::android::hardware::radio::minimal::noError;
using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::network;
namespace aidlRadio = ::aidl::android::hardware::radio;
constexpr auto ok = &ScopedAStatus::ok;

ScopedAStatus RadioNetwork::getDataRegistrationState(int32_t serial) {
    LOG_CALL;

    aidl::CellIdentityLte cellid{
            .mcc = "310",
            .mnc = "555",
            .ci = 12345,
            .pci = 102,
            .tac = 1040,
            .earfcn = 103,
            .operatorNames =
                    {
                            .alphaLong = "Minradio",
                            .alphaShort = "MR",
                            .operatorNumeric = "310555",
                            .status = aidl::OperatorInfo::STATUS_CURRENT,
                    },
            .bandwidth = 1400,
            .additionalPlmns = {},
            .csgInfo = std::nullopt,
            .bands =
                    {
                            aidl::EutranBands::BAND_1,
                            aidl::EutranBands::BAND_88,
                    },
    };
    aidl::RegStateResult res{
            .regState = aidl::RegState::REG_HOME,
            .rat = aidlRadio::RadioTechnology::LTE,
            .reasonForDenial = aidl::RegistrationFailCause::NONE,
            .cellIdentity = cellid,
            .registeredPlmn = "310555",
            .accessTechnologySpecificInfo = aidl::EutranRegistrationInfo{},
    };
    respond()->getDataRegistrationStateResponse(noError(serial), res);
    return ok();
}

ScopedAStatus RadioNetwork::getSignalStrength(int32_t serial) {
    LOG_CALL;

    auto signal = minimal::structs::makeSignalStrength();
    signal.lte = {
            30,   // (0-31, 99)
            100,  // Range: 44 to 140 dBm
            10,   // Range: 20 to 3 dB
            100, 10, RadioConst::VALUE_UNAVAILABLE, RadioConst::VALUE_UNAVAILABLE,
    };

    respond()->getSignalStrengthResponse(noError(serial), signal);
    return ok();
}

}  // namespace android::hardware::radio::service
