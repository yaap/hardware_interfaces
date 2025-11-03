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

#include <aidl/android/hardware/usb/Status.h>
#include <android-base/chrono_utils.h>
#include <android-base/unique_fd.h>

#include <map>
#include <string>

using aidl::android::hardware::usb::Status;
using android::base::unique_fd;
using std::string;
using std::vector;

namespace aidl {
namespace android {
namespace hardware {
namespace usb {

/*
 * addEpollFd - add the given fd to the epollfd(epfd) with the provided events
 */
int addEpollFd(int epfd, int fd, uint32_t events);

/*
 * addEpollFd - create an fd for the file listed in filePath, move it to fildFd, and then add it
 *              to the epollfd(epfd) with the provided events
 */
int addEpollFile(int epfd, const string& filePath, int fileFd, uint32_t events);

/*
 * armTimerFd - arm a timerFd to expire in the given number of milliseconds. Setting the timer
 *              to 0 disarms the timer.
 */
int armTimerFd(int fd, int ms);

/*
 * extractSelection - extract a string from a list of choices where the selected option is enclosed
 *                    in brackets. The substring is saved in selection
 *
 * ex. extractSelection("a [b] c") -> "b"
 */
void extractSelection(string* selection);

/*
 * getTypeCPortNames - extract the port names from the given Type-C path
 */
Status getTypeCPortNames(const char* typecPath, std::map<string, bool>* names);

/*
 * isTypeCPartnerConnected - return whether or not the given local port is connected to a partner
 *                            port
 */
bool isTypeCPartnerConnected(const char* portName);

}  // namespace usb
}  // namespace hardware
}  // namespace android
}  // namespace aidl
