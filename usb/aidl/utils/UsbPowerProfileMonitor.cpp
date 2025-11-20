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

#define LOG_TAG "libusbutils-power"

#include "include/libusbutils/UsbPowerProfileMonitor.h"
#include "include/libusbutils/CommonUtils.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/strings.h>
#include <android_hardware_usb_flags.h>
#include <cutils/uevent.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <utils/Log.h>

#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include <chrono>
#include <regex>
#include <thread>

namespace usb_flags = android::hardware::usb::flags;

using android::base::ParseInt;
using android::base::ReadFileToString;
using android::base::WriteStringToFd;

namespace aidl {
namespace android {
namespace hardware {
namespace usb {

constexpr char kTypecPath[] = "/sys/class/typec/";
constexpr char kUsbPdPath[] = "/sys/class/usb_power_delivery/";

#define BUFFER_MAX_LEN 256

static std::map<Bc12Type, string> bc12_strings = {
        {Bc12Type::UNKNOWN, "unknown"},
        {Bc12Type::SDP, "sdp"},
        {Bc12Type::CDP, "cdp"},
        {Bc12Type::DCP, "dcp"},
};

static std::map<string, int> power_operation_mode_strings = {
        {"default", 500},
        {"1.5A", 1500},
        {"3.0A", 3000},
};

/*---------- File Utilities ----------*/
/**
 * strToBc12 - translate usb_type from the tcpm power supply into Bc12Type
 *
 * usb_type: Unknown SDP [CDP] DCP
 */
static Bc12Type strToBc12(string usbPsyStr) {
    extractSelection(&usbPsyStr);

    if (!strncmp(usbPsyStr.c_str(), "SDP", strlen("SDP"))) {
        return Bc12Type::SDP;
    } else if (!strncmp(usbPsyStr.c_str(), "CDP", strlen("CDP"))) {
        return Bc12Type::CDP;
    } else if (!strncmp(usbPsyStr.c_str(), "DCP", strlen("DCP"))) {
        return Bc12Type::DCP;
    } else {
        return Bc12Type::UNKNOWN;
    }
}

/**
 * getUsbPdDir - get the pd directory from the symbolic link of the local port or partner port's
 * usb_power_delivery directory.
 *
 * ex. usbpd_path = /sys/class/typec/port0/usb_power_delivery
 */
string getUsbPdDir(string usbpd_path) {
    char buf[BUFFER_MAX_LEN];
    int n;
    string symLink;
    std::regex pattern("usb_power_delivery/(pd[0-9]+)");
    std::smatch matches;

    n = readlink(usbpd_path.c_str(), buf, BUFFER_MAX_LEN);
    if (n == -1) {
        ALOGE("error reading usb_power_delivery symbolic link for path %s, errno %d",
              usbpd_path.c_str(), errno);
        return "";
    }
    symLink = string(buf);
    if (std::regex_search(symLink, matches, pattern) && matches.size() == 2) {
        return matches[1].str();
    } else {
        ALOGW("path %s does not have a valid usbpd symLink, %s", usbpd_path.c_str(),
              symLink.c_str());
    }

    return "";
}

/**
 * readPdValue - return the node value for the provided pd path
 *
 * ex. pathPd = "/sys/class/usb_power_delivery/pd0/0:fixed_supply/"
 *     node = "maximum_current"
 */
int readPdValue(string pathPd, string node) {
    string valStr, path;
    int val = 0;
    std::regex pattern("[0-9]+");
    std::smatch matches;

    path = pathPd + node;

    if (!ReadFileToString(path.c_str(), &valStr)) {
        ALOGE("failed to read value at %s", path.c_str());
        return 0;
    }

    if (std::regex_search(valStr, matches, pattern)) {
        if (!ParseInt(matches[0].str(), &val)) {
            ALOGE("could not parse value from %s", matches[0].str().c_str());
        }
    } else {
        ALOGE("value at %s is not formatted properly", path.c_str());
    }

    return val;
}

/*---------- PowerProfileInfo Utilities ----------*/
/**
 * createPowerProfile - create a PowerProfile instance for the provided pathPd and the given
 *                      type. The sink boolean determines what nodes to read for specific power
 *                      profile types.
 *
 * ex. pathPd = "/sys/class/usb_power_delivery/pd0/0:fixed_supply/"
 *     type = "fixed_supply"
 *     sink = true
 */
PowerProfile createPowerProfile(string pathPd, string type, bool sink) {
    PowerProfile profile;

    if (!strncmp(type.c_str(), "fixed_supply", strlen("fixed_supply"))) {
        UsbPdFixed fixedProfile;
        fixedProfile.voltageMv = readPdValue(pathPd, "voltage");
        fixedProfile.maxCurrentMa =
                readPdValue(pathPd, sink ? "operational_current" : "maximum_current");
        profile.set<PowerProfile::fixedProfile>(fixedProfile);
    } else if (!strncmp(type.c_str(), "variable_supply", strlen("variable_supply"))) {
        UsbPdVariable variableProfile;
        variableProfile.minVoltageMv = readPdValue(pathPd, "minimum_voltage");
        variableProfile.maxVoltageMv = readPdValue(pathPd, "maximum_voltage");
        variableProfile.maxCurrentMa =
                readPdValue(pathPd, sink ? "operational_current" : "maximum_current");
        profile.set<PowerProfile::variableProfile>(variableProfile);
    } else if (!strncmp(type.c_str(), "battery", strlen("battery"))) {
        UsbPdBattery batteryProfile;
        batteryProfile.minVoltageMv = readPdValue(pathPd, "minimum_voltage");
        batteryProfile.maxVoltageMv = readPdValue(pathPd, "maximum_voltage");
        batteryProfile.maxPowerMw =
                readPdValue(pathPd, sink ? "operational_power" : "maximum_power");
        profile.set<PowerProfile::batteryProfile>(batteryProfile);
    } else if (!strncmp(type.c_str(), "programmable_supply", strlen("programmable_supply"))) {
        UsbPdSprPps sprPpsProfile;
        sprPpsProfile.minVoltageMv = readPdValue(pathPd, "minimum_voltage");
        sprPpsProfile.maxVoltageMv = readPdValue(pathPd, "maximum_voltage");
        sprPpsProfile.maxCurrentMa = readPdValue(pathPd, "maximum_current");
        profile.set<PowerProfile::sprPpsProfile>(sprPpsProfile);
    } else if (!strncmp(type.c_str(), "spr_adjustable_voltage_supply",
                        strlen("spr_adjustable_voltage_supply"))) {
        UsbPdSprAvs sprAvsProfile;
        sprAvsProfile.maxCurrent15vMa = readPdValue(pathPd, "maximum_current_9V_to_15V");
        sprAvsProfile.maxCurrent20vMa = readPdValue(pathPd, "maximum_current_15V_to_20V");
        profile.set<PowerProfile::sprAvsProfile>(sprAvsProfile);
    } else if (!strncmp(type.c_str(), "typec_default", strlen("typec_default"))) {
        TypecDefault typecDefaultProfile;
        typecDefaultProfile.maxCurrentMa = 500;
        profile.set<PowerProfile::typecDefaultProfile>(typecDefaultProfile);
    } else if (!strncmp(type.c_str(), "typec_1.5A", strlen("typec_1.5A"))) {
        profile.set<PowerProfile::typec15AProfile>(true);
    } else if (!strncmp(type.c_str(), "typec_3.0A", strlen("typec_3.0A"))) {
        profile.set<PowerProfile::typec30AProfile>(true);
    }

    return profile;
}

/**
 * createTypecProfiles - adds Type-C PowerProfile objects to profiles based on
 * the maximum current supported.
 */
void createTypecProfiles(std::vector<PowerProfile>* profiles, int maxCurrentMa) {
    PowerProfile profile;

    if (maxCurrentMa >= 500) {
        profile = createPowerProfile("", "typec_default", false);
        (*profiles).push_back(profile);
        ALOGI("adding profile %s", profile.toString().c_str());
    }
    if (maxCurrentMa >= 1500) {
        profile = createPowerProfile("", "typec_1.5A", false);
        (*profiles).push_back(profile);
        ALOGI("adding profile %s", profile.toString().c_str());
    }
    if (maxCurrentMa >= 3000) {
        profile = createPowerProfile("", "typec_3.0A", false);
        (*profiles).push_back(profile);
        ALOGI("adding profile %s", profile.toString().c_str());
    }
}

/**
 * matchPowerProfiles - performs matching on portProfiles to partnerProfiles
 */
void matchPowerProfiles(const std::vector<PowerProfile>& portProfiles,
                        const std::vector<PowerProfile>& partnerProfiles,
                        std::vector<PowerProfileMatchResult>* matchResults,
                        ProfileType portProfileType) {
    bool match;
    /* Voltage */
    int portMinVoltageMv, partnerMinVoltageMv;
    int portMaxVoltageMv, partnerMaxVoltageMv;
    /* Current */
    int portMaxCurrentMa, partnerMaxCurrentMa;
    /* AVS */
    int portMaxCurrent15vMa, portMaxCurrent20vMa;
    int partnerMaxCurrent15vMa, partnerMaxCurrent20vMa;
    /* PowerProfile info objects */
    UsbPdFixed portFixed, partnerFixed;
    UsbPdSprPps portSprPps, partnerSprPps;
    UsbPdSprAvs portSprAvs, partnerSprAvs;

    /* Perform initial matching based on voltage equivalents */
    for (int i = 0; i < portProfiles.size(); i++) {
        for (int j = 0; j < partnerProfiles.size(); j++) {
            if (portProfiles[i].getTag() == partnerProfiles[j].getTag()) {
                PowerProfileMatchResult matchResult;
                PowerProfile matchProfile;
                TypecDefault typecDefaultProfile;
                UsbPdFixed fixedProfile;
                UsbPdSprPps sprPpsProfile;
                UsbPdSprAvs sprAvsProfile;
                match = true;

                switch (portProfiles[i].getTag()) {
                    case PowerProfile::typecDefaultProfile:
                        typecDefaultProfile.maxCurrentMa = 500;
                        matchProfile.set<PowerProfile::typecDefaultProfile>(typecDefaultProfile);
                        break;
                    case PowerProfile::typec15AProfile:
                        matchProfile.set<PowerProfile::typec15AProfile>(true);
                        break;
                    case PowerProfile::typec30AProfile:
                        matchProfile.set<PowerProfile::typec30AProfile>(true);
                        break;
                    case PowerProfile::fixedProfile:
                        portFixed = portProfiles[i].get<PowerProfile::fixedProfile>();
                        partnerFixed = partnerProfiles[j].get<PowerProfile::fixedProfile>();

                        portMaxVoltageMv = portFixed.voltageMv;
                        partnerMaxVoltageMv = partnerFixed.voltageMv;
                        partnerMaxCurrentMa = partnerFixed.maxCurrentMa;

                        if (portMaxVoltageMv != partnerMaxVoltageMv) {
                            match = false;
                            break;
                        }

                        fixedProfile.voltageMv = partnerMaxVoltageMv;
                        fixedProfile.maxCurrentMa =
                                std::min(portFixed.maxCurrentMa, partnerMaxCurrentMa);
                        matchProfile.set<PowerProfile::fixedProfile>(fixedProfile);
                        break;
                    case PowerProfile::sprPpsProfile:
                        portSprPps = portProfiles[i].get<PowerProfile::sprPpsProfile>();
                        partnerSprPps = partnerProfiles[j].get<PowerProfile::sprPpsProfile>();

                        portMinVoltageMv = portSprPps.minVoltageMv;
                        portMaxVoltageMv = portSprPps.maxVoltageMv;
                        portMaxCurrentMa = portSprPps.maxCurrentMa;

                        partnerMinVoltageMv = partnerSprPps.minVoltageMv;
                        partnerMaxVoltageMv = partnerSprPps.maxVoltageMv;
                        partnerMaxCurrentMa = partnerSprPps.maxCurrentMa;

                        /* The PPS Sink range needs to be fully covered by the PPS Source */
                        if (portProfileType == PORT_SINK) {
                            if (portMinVoltageMv < partnerMinVoltageMv ||
                                portMaxVoltageMv > partnerMaxVoltageMv) {
                                match = false;
                                break;
                            }
                        } else {
                            if (partnerMinVoltageMv < portMinVoltageMv ||
                                partnerMaxVoltageMv > portMaxVoltageMv) {
                                match = false;
                                break;
                            }
                        }

                        sprPpsProfile.minVoltageMv =
                                std::max(portMinVoltageMv, partnerMinVoltageMv);
                        sprPpsProfile.maxVoltageMv = partnerMaxVoltageMv;
                        sprPpsProfile.maxCurrentMa =
                                std::min(portMaxCurrentMa, partnerMaxCurrentMa);
                        matchProfile.set<PowerProfile::sprPpsProfile>(sprPpsProfile);
                        break;
                    case PowerProfile::sprAvsProfile:
                        portSprAvs = portProfiles[i].get<PowerProfile::sprAvsProfile>();
                        partnerSprAvs = partnerProfiles[j].get<PowerProfile::sprAvsProfile>();
                        portMaxCurrent15vMa = portSprAvs.maxCurrent15vMa;
                        partnerMaxCurrent15vMa = partnerSprAvs.maxCurrent15vMa;
                        portMaxCurrent20vMa = portSprAvs.maxCurrent20vMa;
                        partnerMaxCurrent20vMa = partnerSprAvs.maxCurrent20vMa;
                        if (portMaxCurrent15vMa > 0 && partnerMaxCurrent15vMa > 0) {
                            sprAvsProfile.maxCurrent15vMa =
                                    std::min(portMaxCurrent15vMa, partnerMaxCurrent15vMa);
                            matchProfile.set<PowerProfile::sprAvsProfile>(sprAvsProfile);
                        } else if (portMaxCurrent20vMa > 0 && partnerMaxCurrent20vMa > 0) {
                            sprAvsProfile.maxCurrent20vMa =
                                    std::min(portMaxCurrent20vMa, partnerMaxCurrent20vMa);
                            matchProfile.set<PowerProfile::sprAvsProfile>(sprAvsProfile);
                        } else {
                            match = false;
                        }
                        break;
                    default:
                        match = false;
                        break;
                }
                if (match) {
                    matchResult.portIndex = i;
                    matchResult.partnerIndex = j;
                    matchResult.result = matchProfile;
                    (*matchResults).push_back(matchResult);
                }
            }
        }
    }
}

/**
 * handleAvsPowerProfileCreation - AVS PDOs report special fields maxCurrent15vMa and
 *                                 maxCurrent20vMa. In order to translate them properly at the
 *                                 frameworks layer, we split an AVS PDO into two power profiles
 *                                 that cover the 9V-15V range and the 15V-20V range separately.
 */
void handleAvsPowerProfileCreation(PowerProfile* profile, std::vector<PowerProfile>* profiles) {
    PowerProfile newProfile;
    UsbPdSprAvs avsCurr, avsNew;

    if ((*profile).getTag() != PowerProfile::sprAvsProfile) {
        return;
    }

    /* Get AVS profile */
    avsCurr = (*profile).get<PowerProfile::sprAvsProfile>();

    /* If both fields are valid, then split into two AVS profiles */
    if (avsCurr.maxCurrent15vMa > 0 && avsCurr.maxCurrent20vMa > 0) {
        /* New profile will inherit 15V range current */
        avsNew.maxCurrent15vMa = avsCurr.maxCurrent15vMa;
        newProfile.set<PowerProfile::sprAvsProfile>(avsNew);

        /* Old profile will only cover 20V+ case */
        avsCurr.maxCurrent15vMa = 0;
        (*profile).set<PowerProfile::sprAvsProfile>(avsCurr);

        /* Add new profile to profiles */
        (*profiles).push_back(newProfile);
        ALOGI("adding profile %s", newProfile.toString().c_str());
    }
}

/*---------- UsbPowerProfileMonitor Methods ----------*/
/**
 * getBc12Type - get the partner BC 1.2 Type
 *
 * @param portName          The id of port to query BC 1.2 type for
 */
Bc12Type UsbPowerProfileMonitor::getBc12Type(string portName) {
    string usbPsy, usbPsyPath;
    Bc12Type bc12Type = Bc12Type::UNKNOWN;

    if (!mSupportsPartnerBc12Reporting) {
        return Bc12Type::UNKNOWN;
    }

    usbPsyPath = kTypecPath + portName + "/device/power_supply/usb/usb_type";
    if (!ReadFileToString(usbPsyPath.c_str(), &usbPsy)) {
        ALOGE("failed to read usb psy usb mode at %s", usbPsyPath.c_str());
        return Bc12Type::UNKNOWN;
    }

    bc12Type = strToBc12(usbPsy);

    ALOGI("port %s bc12type %s", portName.c_str(), bc12_strings[bc12Type].c_str());

    return bc12Type;
}

/**
 * populateTypecProfiles - populates profiles with Type-C PowerProfiles for the given port
 *
 * @param portName          The id of port to populate Type-C PowerProfiles for
 * @param profiles          The PowerProfile vector to update
 */
void UsbPowerProfileMonitor::populateTypecProfiles(string portName,
                                                   std::vector<PowerProfile>* profiles) {
    bool pdSupported = false;
    UsbPdFixed profileFixed;
    int maxCurrentMa = 0;

    for (PowerProfile profile : (*profiles)) {
        if (profile.getTag() == PowerProfile::fixedProfile) {
            profileFixed = profile.get<PowerProfile::fixedProfile>();
            if (profileFixed.voltageMv == 5000) {
                pdSupported = true;
                maxCurrentMa = profileFixed.maxCurrentMa;
                break;
            }
        }
    }

    if (pdSupported) {
        createTypecProfiles(profiles, maxCurrentMa);
    } else {
        /* Verify that partner is connected */
        if (!isTypeCPartnerConnected(portName.c_str())) {
            return;
        }

        /* Extract string from power_operation_mode */
        string powerOp, powerOpPath;
        powerOpPath = kTypecPath + portName + "/power_operation_mode";
        if (!ReadFileToString(powerOpPath.c_str(), &powerOp)) {
            ALOGE("failed to read power_operation_mode at %s", powerOpPath.c_str());
            return;
        }

        if (power_operation_mode_strings.find(powerOp) == power_operation_mode_strings.end()) {
            ALOGW("power_operation_mode for port %s is PD", portName.c_str());
            return;
        }

        maxCurrentMa = power_operation_mode_strings[powerOp];
        createTypecProfiles(profiles, maxCurrentMa);
    }
}

/**
 * populatePowerProfiles - populates profiles with PowerProfile objects for a given port
 *
 * @param portName          The id of port to populate PowerProfiles for
 * @param profiles          The PowerProfile vector to update
 * @param profileType       Describes which PortStatus field is provided in @profiles
 */
void UsbPowerProfileMonitor::populatePowerProfiles(string portName,
                                                   std::vector<PowerProfile>* profiles,
                                                   ProfileType profileType) {
    UsbPortInfo portInfo = mUsbPortInfo[portName];
    DIR* dirCaps;
    string pathCaps, pathPd, namePd, nameDt, strCaps;
    struct dirent* ep;
    std::regex pattern("[0-9]+:(.+)");
    std::smatch matches;
    PowerProfile profile, profileExtra;
    bool isSink = false;

    switch (profileType) {
        case ProfileType::PORT_SINK:
            namePd = portInfo.portPdName;
            strCaps = "sink-capabilities";
            isSink = true;
            break;
        case ProfileType::PORT_SOURCE:
            namePd = portInfo.portPdName;
            strCaps = "source-capabilities";
            isSink = false;
            break;
        case ProfileType::PARTNER_SINK:
            namePd = portInfo.partnerPdName;
            strCaps = "sink-capabilities";
            isSink = true;
            break;
        case ProfileType::PARTNER_SOURCE:
            namePd = portInfo.partnerPdName;
            strCaps = "source-capabilities";
            isSink = false;
            break;
    }

    /* Handle PD Profiles */
    if (namePd.empty()) {
        ALOGI("%s: %s is missing pd directory", __func__,
              (profileType == PORT_SINK || profileType == PORT_SOURCE) ? "port" : "partner");
        goto populate_typec;
    }

    pathCaps = kUsbPdPath + namePd + "/" + strCaps + "/";
    dirCaps = opendir(pathCaps.c_str());
    if (!dirCaps) {
        ALOGE("%s: couldn't open %s", __func__, pathCaps.c_str());
        return;
    }
    while ((ep = readdir(dirCaps))) {
        if (ep->d_type == DT_DIR) {
            nameDt = string(ep->d_name);
            if (std::regex_search(nameDt, matches, pattern) && matches.size() == 2) {
                pathPd = pathCaps + nameDt + "/";
                profile = createPowerProfile(pathPd, matches[1].str(), isSink);
                handleAvsPowerProfileCreation(&profile, profiles);
                ALOGI("adding profile %s", profile.toString().c_str());
                (*profiles).push_back(profile);
            }
        }
    }
    closedir(dirCaps);

populate_typec:
    populateTypecProfiles(portName, profiles);
}

/**
 * updatePowerProfiles - updates the portStatus PowerProfile relevant fields
 *
 * @param portName          The id of port to update PowerProfile relevant arrays in portStatus
 * @param portStatus        The PortStatus to update PowerProfile relevant arrays for
 */
void UsbPowerProfileMonitor::updatePowerProfiles(string portName, PortStatus* portStatus) {
    string pathUsbpd;

    if (!mSupportsPowerProfiles) {
        return;
    }

    /* Handle Power Profile Event gets the partner and usbpd path */
    /* Get local port pd path */
    pathUsbpd = kTypecPath + portName + "/usb_power_delivery";
    mUsbPortInfo[portName].portPdName = getUsbPdDir(pathUsbpd);
    /* Get partner port pd path */
    pathUsbpd = kTypecPath + portName + "-partner/usb_power_delivery";
    mUsbPortInfo[portName].partnerPdName = getUsbPdDir(pathUsbpd);

    /* Populate local port sink power profiles */
    ALOGI("%s: populating local port sink power profiles", __func__);
    populatePowerProfiles(portName, &portStatus->sinkPowerProfiles, PORT_SINK);
    ALOGI("%s: populating local port source power profiles", __func__);
    populatePowerProfiles(portName, &portStatus->sourcePowerProfiles, PORT_SOURCE);
    ALOGI("%s: populating partner port sink power profiles", __func__);
    populatePowerProfiles(portName, &portStatus->partnerStatus->sinkPowerProfiles, PARTNER_SINK);
    ALOGI("%s: populating partner port source power profiles", __func__);
    populatePowerProfiles(portName, &portStatus->partnerStatus->sourcePowerProfiles,
                          PARTNER_SOURCE);

    /* Perform Matching */
    matchPowerProfiles(portStatus->sinkPowerProfiles,
                       portStatus->partnerStatus->sourcePowerProfiles,
                       &portStatus->sinkMatchResults, PORT_SINK);
    matchPowerProfiles(portStatus->sourcePowerProfiles,
                       portStatus->partnerStatus->sinkPowerProfiles,
                       &portStatus->sourceMatchResults, PORT_SOURCE);
}

UsbPowerProfileMonitor::UsbPowerProfileMonitor(bool supportsPartnerBc12Reporting,
                                               bool supportPowerProfiles) {
    std::map<string, bool> names;

    mSupportsPartnerBc12Reporting = supportsPartnerBc12Reporting;
    mSupportsPowerProfiles = supportPowerProfiles;

    getTypeCPortNames(kTypecPath, &names);
    for (std::pair<string, bool> port : names) {
        mUsbPortInfo[port.first].portPdName = "";
        mUsbPortInfo[port.first].partnerPdName = "";
    }

    unique_fd timerDebounceFd(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK));
    if (timerDebounceFd.get() == -1) {
        ALOGE("create timerDebounceFd failed");
        return;
    }

    mTimerDebounceFd = std::move(timerDebounceFd);
}

/**
 * queryPowerProfileStatus - callback used by the USB HAL to update the currentPortStatus with
 *                           the current BC 1.2 and PowerProfile information
 *
 * @param currentPortStatus         The array of PortStatus objects to update
 */
void UsbPowerProfileMonitor::queryPowerProfileStatus(std::vector<PortStatus>* currentPortStatus) {
    if (!usb_flags::enable_power_profile_reporting()) {
        return;
    }

    for (int i = 0; i < currentPortStatus->size(); i++) {
        string portName = (*currentPortStatus)[i].portName;
        if (mUsbPortInfo.find(portName) == mUsbPortInfo.end()) {
            ALOGE("UsbPowerProfileMonitor does not have port named %s", portName.c_str());
            continue;
        }

        (*currentPortStatus)[i].supportsPartnerBc12Type = mSupportsPartnerBc12Reporting;
        (*currentPortStatus)[i].supportsPowerProfiles = mSupportsPowerProfiles;

        (*currentPortStatus)[i].partnerStatus->bc12Type = getBc12Type(portName);
        updatePowerProfiles(portName, &(*currentPortStatus)[i]);
    }
}

}  // namespace usb
}  // namespace hardware
}  // namespace android
}  // namespace aidl
