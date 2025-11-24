/*
 * Copyright (C) 2025 The Android Open Source Project
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

#include <aidl/android/hardware/usb/PortStatus.h>
#include <android-base/unique_fd.h>
#include <sys/epoll.h>

#include <functional>
#include <map>
#include <string>
#include <vector>

using aidl::android::hardware::usb::Bc12Type;
using aidl::android::hardware::usb::PortStatus;
using aidl::android::hardware::usb::PowerProfile;
using aidl::android::hardware::usb::PowerProfileMatchResult;

using std::string;

using android::base::unique_fd;

#define POWER_PROFILE_MAX_NUM 16

#define POWER_MONITOR_DEBOUNCE_MS 1500

namespace aidl {
namespace android {
namespace hardware {
namespace usb {

enum ProfileType {
    PORT_SINK,
    PORT_SOURCE,
    PARTNER_SINK,
    PARTNER_SOURCE,
};

class UsbPowerProfileMonitor {
  public:
    UsbPowerProfileMonitor(bool supportsPartnerBc12Reporting, bool supportPowerProfiles);

    unique_fd mTimerDebounceFd;

    void queryPowerProfileStatus(std::vector<PortStatus>* currentPortStatus);

  private:
    struct UsbPortInfo {
        string portPdName;
        string partnerPdName;
    };

    std::map<string, UsbPortInfo> mUsbPortInfo;

    /*
     * Port reporting capabilities
     */
    bool mSupportsPartnerBc12Reporting;
    bool mSupportsPowerProfiles;

    Bc12Type getBc12Type(string portName);

    void populatePowerProfiles(string portName, std::vector<PowerProfile>* profiles,
                               ProfileType profileType);
    void populateTypecProfiles(string portName, std::vector<PowerProfile>* profiles);
    void handlePowerProfileEvent(bool remove, string pdName);
    void updateBc12State();
    void updatePowerProfiles(string portName, PortStatus* portStatus);
};

}  // namespace usb
}  // namespace hardware
}  // namespace android
}  // namespace aidl
