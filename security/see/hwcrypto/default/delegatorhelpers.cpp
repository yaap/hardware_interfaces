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

#include "delegatorhelpers.h"
#include <android-base/logging.h>
#include <getopt.h>

static void showUsageAndExit(int code, const char* app_name) {
    LOG(ERROR) << "usage: " << app_name << " -d <trusty_dev>";
    exit(code);
}

void parseDeviceName(int argc, char* argv[], char*& device_name, const char* app_name) {
    static const char* _sopts = "h:d:";
    static const struct option _lopts[] = {{"help", no_argument, nullptr, 'h'},
                                           {"trusty_dev", required_argument, nullptr, 'd'},
                                           {0, 0, 0, 0}};
    int opt;
    int oidx = 0;

    while ((opt = getopt_long(argc, argv, _sopts, _lopts, &oidx)) != -1) {
        switch (opt) {
            case 'd':
                device_name = strdup(optarg);
                break;
            case 'h':
                showUsageAndExit(EXIT_SUCCESS, app_name);
                break;
            default:
                LOG(ERROR) << "unrecognized option: " << opt;
                showUsageAndExit(EXIT_FAILURE, app_name);
        }
    }

    if (device_name == nullptr) {
        LOG(ERROR) << "missing required argument(s)";
        showUsageAndExit(EXIT_FAILURE, app_name);
    }

    LOG(INFO) << "starting " << app_name;
    LOG(INFO) << "trusty dev: " << device_name;
}