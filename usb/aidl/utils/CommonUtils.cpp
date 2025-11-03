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

#define LOG_TAG "libusbutils-common"

#include "include/libusbutils/CommonUtils.h"

#include <android-base/file.h>
#include <android-base/properties.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <sys/timerfd.h>
#include <utils/Log.h>

#include <dirent.h>
#include <stdio.h>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>

namespace aidl {
namespace android {
namespace hardware {
namespace usb {

constexpr char kTypecPath[] = "/sys/class/typec/";
constexpr char kUsbPdPath[] = "/sys/class/usb_power_delivery/";

/*------------------------- Epoll Utility Functions --------------------------*/
int addEpollFd(int epfd, int fd, uint32_t events) {
    struct epoll_event event;
    int ret;

    event.data.fd = fd;
    event.events = events;

    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
    if (ret) ALOGE("epoll_ctl error %d", errno);

    return ret;
}

int addEpollFile(int epfd, const string& filePath, int fileFd, uint32_t events) {
    struct epoll_event ev;

    unique_fd fd(open(filePath.c_str(), O_RDONLY));

    if (fd.get() == -1) {
        ALOGI("Cannot open %s", filePath.c_str());
        return -1;
    }

    ev.data.fd = fd.get();
    ev.events = events;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd.get(), &ev) != 0) {
        ALOGE("epoll_ctl failed; errno=%d", errno);
        return -1;
    }

    fileFd = std::move(fd);
    ALOGI("epoll registered %s", filePath.c_str());
    return 0;
}

int armTimerFd(int fd, int ms) {
    struct itimerspec ts;
    int ret;

    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;
    ts.it_value.tv_sec = ms / 1000;
    ts.it_value.tv_nsec = (ms % 1000) * 1000000;

    ret = timerfd_settime(fd, 0, &ts, NULL);
    if (ret < 0) {
        ALOGE("%s failed to arm timer", __func__);
    }

    return ret;
}

/*------------------------- String Utility Functions -------------------------*/
void extractSelection(string* selection) {
    std::size_t first, last;

    first = selection->find("[");
    last = selection->find("]");

    if (first != string::npos && last != string::npos) {
        *selection = selection->substr(first + 1, last - first - 1);
    }
}

/*--------------------------- Type-C Helpers ---------------------------*/
Status getTypeCPortNames(const char* typecPath, std::map<string, bool>* names) {
    DIR* dp;

    dp = opendir(typecPath);
    if (dp != NULL) {
        struct dirent* ep;

        while ((ep = readdir(dp))) {
            if (ep->d_type == DT_LNK) {
                if (string::npos == string(ep->d_name).find("-partner")) {
                    std::map<string, bool>::const_iterator portName = names->find(ep->d_name);
                    if (portName == names->end()) {
                        names->insert({ep->d_name, false});
                    }
                } else {
                    (*names)[std::strtok(ep->d_name, "-")] = true;
                }
            }
        }
        closedir(dp);
        return Status::SUCCESS;
    }

    ALOGE("Failed to open /sys/class/typec");
    return Status::ERROR;
}

bool isTypeCPartnerConnected(const char* portName) {
    DIR* dp;

    string path = kTypecPath + string(portName) + "-partner";

    dp = opendir(path.c_str());
    if (dp != NULL) {
        closedir(dp);
        return true;
    } else {
        return false;
    }
}

}  // namespace usb
}  // namespace hardware
}  // namespace android
}  // namespace aidl