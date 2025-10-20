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
#include <android-base/result.h>
#include <binder/RpcSession.h>
#include <binder/RpcTrusty.h>
#include <trusty/tipc.h>
#include "delegatorhelpers.h"
#include "hwcryptosharedsecretimpl.h"

using android::base::Result;
using android::binder::Status;

#define SHARED_SECRET_PORT "android.hardware.security.hwcrypto.sharedsecret/default.bnd"

namespace android {
namespace trusty {
namespace hwcryptohalservice {

HwCryptoSharedSecret::HwCryptoSharedSecret() {}

Result<void> HwCryptoSharedSecret::connectToTrusty(const char* tipcDev) {
    return connectToTrustyService(tipcDev, SHARED_SECRET_PORT, mSession, mRoot,
                                  mSharedSecretServer);
}

std::shared_ptr<HwCryptoSharedSecret> HwCryptoSharedSecret::Create(const char* tipcDev) {
    std::shared_ptr<HwCryptoSharedSecret> sharedSecret =
            ndk::SharedRefBase::make<HwCryptoSharedSecret>();

    if (!sharedSecret) {
        LOG(ERROR) << "failed to allocate HwCryptoSharedSecret";
        return nullptr;
    }

    auto ret = sharedSecret->connectToTrusty(tipcDev);
    if (!ret.ok()) {
        LOG(ERROR) << "failed to connect HwCryptoSharedSecret to Trusty: " << ret.error();
        return nullptr;
    }

    return sharedSecret;
}

ndk::ScopedAStatus HwCryptoSharedSecret::getSharedSecretParameters(
        ndk_sharedsecret::SharedSecretParameters* aidl_return) {
    cpp_sharedsecret::SharedSecretParameters binder_return;
    auto status = mSharedSecretServer->getSharedSecretParameters(&binder_return);
    if (status.isOk() && (aidl_return != nullptr)) {
        aidl_return->seed = binder_return.seed;
        aidl_return->nonce = binder_return.nonce;
    }
    return convertStatus(status);
}

ndk::ScopedAStatus HwCryptoSharedSecret::computeSharedSecret(
        const std::vector<ndk_sharedsecret::SharedSecretParameters>& params,
        std::vector<uint8_t>* aidl_return) {
    auto status = Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
    if (aidl_return == nullptr) {
        return convertStatus(status);
    }
    std::vector<cpp_sharedsecret::SharedSecretParameters> cpp_params;
    for (const ndk_sharedsecret::SharedSecretParameters& param : params) {
        cpp_sharedsecret::SharedSecretParameters cpp_param;
        cpp_param.seed = param.seed;
        cpp_param.nonce = param.nonce;
        cpp_params.push_back(std::move(cpp_param));
    }
    status = mSharedSecretServer->computeSharedSecret(cpp_params, aidl_return);

    return convertStatus(status);
}

}  // namespace hwcryptohalservice
}  // namespace trusty
}  // namespace android
