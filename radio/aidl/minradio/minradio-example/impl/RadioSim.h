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
#pragma once

#include <libminradio/sim/RadioSim.h>

namespace android::hardware::radio::service {

class RadioSim : public minimal::RadioSim {
  public:
    RadioSim(std::shared_ptr<minimal::SlotContext> context);

  protected:
    ::ndk::ScopedAStatus getIccCardStatus(int32_t serial) override;
    ::ndk::ScopedAStatus getImsiForApp(int32_t serial, const std::string& aid) override;
};

}  // namespace android::hardware::radio::service
