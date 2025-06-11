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

#include "RadioData.h"

#include <aidl/android/hardware/radio/RadioConst.h>
#include <libminradio/debug.h>
#include <libminradio/response.h>
#include <libnetdevice/libnetdevice.h>

#define RADIO_MODULE "DataImpl"

namespace android::hardware::radio::service {

using namespace ::android::hardware::radio::minimal::binder_printing;
using ::aidl::android::hardware::radio::RadioConst;
using ::aidl::android::hardware::radio::RadioError;
using ::android::hardware::radio::minimal::errorResponse;
using ::android::hardware::radio::minimal::noError;
using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::data;
namespace aidlCommon = ::aidl::android::hardware::radio;
constexpr auto ok = &ScopedAStatus::ok;

ScopedAStatus RadioData::setupDataCall(int32_t serial, aidlCommon::AccessNetwork accessNetwork,
                                       const aidl::DataProfileInfo& dataProfileInfo,
                                       bool roamingAllowed, aidl::DataRequestReason reason,
                                       const std::vector<aidl::LinkAddress>& addresses,
                                       const std::vector<std::string>& dnses, int32_t pduSessId,
                                       const std::optional<aidl::SliceInfo>& sliceInfo,
                                       bool matchAllRuleAllowed) {
    LOG_CALL << accessNetwork                             //
             << " {" << dataProfileInfo.profileId << '}'  //
             << ' ' << roamingAllowed                     //
             << ' ' << reason                             //
             << ' ' << addresses.size()                   //
             << ' ' << dnses.size() << ' ' << pduSessId   //
             << ' ' << sliceInfo.has_value()              //
             << ' ' << matchAllRuleAllowed;

    bool ifaceOk = netdevice::setAddr4("buried_eth0", "192.168.97.2", 30);
    ifaceOk = ifaceOk && netdevice::up("buried_eth0");
    if (!ifaceOk) {
        respond()->setupDataCallResponse(errorResponse(serial, RadioError::INTERNAL_ERR), {});
        return ok();
    }

    aidl::SetupDataCallResult result{
            .cause = aidl::DataCallFailCause::NONE,
            .suggestedRetryTime = RadioConst::VALUE_UNAVAILABLE_LONG,
            .cid = setupDataCallCid(),
            .active = aidl::SetupDataCallResult::DATA_CONNECTION_STATUS_ACTIVE,
            .type = aidl::PdpProtocolType::IP,
            .ifname = "buried_eth0",
            .addresses = {{
                    .address = "192.168.97.2/30",
                    .addressProperties = 0,
                    .deprecationTime = RadioConst::VALUE_UNAVAILABLE_LONG,
                    .expirationTime = RadioConst::VALUE_UNAVAILABLE_LONG,
            }},
            .dnses = {"8.8.8.8"},
            .gateways = {"192.168.97.1"},
            .pcscf = {},
            .mtuV4 = 0,
            .mtuV6 = 0,
            .defaultQos = {},
            .qosSessions = {},
            .handoverFailureMode = aidl::SetupDataCallResult::HANDOVER_FAILURE_MODE_LEGACY,
            .pduSessionId = 0,
            .sliceInfo = std::nullopt,
            .trafficDescriptors = {},
    };

    setupDataCallBase(result);

    respond()->setupDataCallResponse(noError(serial), result);
    return ok();
}

}  // namespace android::hardware::radio::service
