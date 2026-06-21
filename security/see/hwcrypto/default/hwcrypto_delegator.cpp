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

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <getopt.h>
#include <string>
#include "delegatorhelpers.h"
#include "hwcryptokeyimpl.h"
#include "hwcryptosharedsecretimpl.h"

const char* APP_NAME = "android.hardware.trusty.hwcryptohal-service";
const char* REGISTER_SHARED_SECRET = "ro.vendor.trusty.hwcryptohal.register_shared_secret";

bool register_hwcrypto_isharedsecret() {
    auto register_shared_secret = android::base::GetBoolProperty(REGISTER_SHARED_SECRET, false);
    return register_shared_secret;
}

int main(int argc, char* argv[]) {
    char* device_name;
    parseDeviceName(argc, argv, device_name, APP_NAME);

    auto hwCryptoServer = android::trusty::hwcryptohalservice::HwCryptoKey::Create(device_name);
    if (hwCryptoServer == nullptr) {
        LOG(ERROR) << "couldn't create hwcrypto service";
        exit(EXIT_FAILURE);
    }
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    const std::string instance =
            std::string() + ndk_hwcrypto::IHwCryptoKey::descriptor + "/default";
    binder_status_t status =
            AServiceManager_addService(hwCryptoServer->asBinder().get(), instance.c_str());
    if (status != STATUS_OK) {
        LOG(ERROR) << "couldn't register hwcrypto service";
    }
    CHECK_EQ(status, STATUS_OK);

    if (register_hwcrypto_isharedsecret()) {
        auto sharedSecretServer =
                android::trusty::hwcryptohalservice::HwCryptoSharedSecret::Create(device_name);
        if (sharedSecretServer == nullptr) {
            LOG(ERROR) << "couldn't create hwcrypto shared secret service";
            exit(EXIT_FAILURE);
        }
        const std::string sharedSecretInstance =
                std::string() + ndk_sharedsecret::ISharedSecret::descriptor + "/hwcrypto";
        status = AServiceManager_addService(sharedSecretServer->asBinder().get(),
                                            sharedSecretInstance.c_str());
        if (status != STATUS_OK) {
            LOG(ERROR) << "couldn't register hwcrypto shared secret service";
        }
        CHECK_EQ(status, STATUS_OK);
    }

    ABinderProcess_joinThreadPool();

    return 0;
}
