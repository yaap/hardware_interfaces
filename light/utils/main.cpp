/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <iostream>
#include <string>

#include <android-base/logging.h>
#include <android/hardware/light/ILights.h>
#include <binder/IServiceManager.h>

using android::sp;
using android::waitForVintfService;
using android::binder::Status;

using android::hardware::light::BrightnessMode;
using android::hardware::light::FlashMode;
using android::hardware::light::HwLight;
using android::hardware::light::HwLightState;
using android::hardware::light::ILights;

void error(const std::string& msg) {
    LOG(ERROR) << msg;
    std::cerr << msg << std::endl;
}

int parseArgs(int argc, char* argv[], unsigned int* color) {
    if (argc > 2) {
        error("Usage: blank_screen [color]");
        return -1;
    }

    if (argc > 1) {
        char* col_ptr;

        *color = strtoul(argv[1], &col_ptr, 0);
        if (*col_ptr != '\0') {
            error("Failed to convert " + std::string(argv[1]) + " to number");
            return -1;
        }

        return 0;
    }

    *color = 0u;
    return 0;
}

void setToColor(sp<ILights> hal, unsigned int color) {
    static HwLightState off;
    off.color = color;
    off.flashMode = FlashMode::NONE;
    off.brightnessMode = BrightnessMode::USER;

    std::vector<HwLight> lights;
    Status status = hal->getLights(&lights);
    if (!status.isOk()) {
        error("Failed to list lights");
        return;
    }

    for (auto light : lights) {
        Status setStatus = hal->setLightState(light.id, off);
        if (!setStatus.isOk()) {
            error("Failed to shut off light id " + std::to_string(light.id));
        }
    }
}

int main(int argc, char* argv[]) {
    unsigned int inputColor;
    int result = parseArgs(argc, argv, &inputColor);
    if (result != 0) {
        return result;
    }

    auto hal = waitForVintfService<ILights>();
    if (hal != nullptr) {
        setToColor(hal, inputColor);
        return 0;
    }

    error("Could not retrieve light service.");
    return -1;
}
