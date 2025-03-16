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

#include "RadioModem.h"

#include <libminradio/debug.h>
#include <libminradio/response.h>

#define RADIO_MODULE "ModemImpl"

namespace android::hardware::radio::service {

using ::aidl::android::hardware::radio::RadioTechnology;
using ::android::hardware::radio::minimal::noError;
using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::modem;
constexpr auto ok = &ScopedAStatus::ok;

RadioModem::RadioModem(std::shared_ptr<minimal::SlotContext> context)
    : minimal::RadioModem(context, {{RadioTechnology::LTE, RadioTechnology::HSPA}}) {}

ScopedAStatus RadioModem::getImei(int32_t serial) {
    LOG_CALL;
    aidl::ImeiInfo info{
            .type = aidl::ImeiInfo::ImeiType::PRIMARY,
            .imei = "867400022047199",
            .svn = "01",
    };
    respond()->getImeiResponse(noError(serial), info);
    return ok();
}

}  // namespace android::hardware::radio::service
