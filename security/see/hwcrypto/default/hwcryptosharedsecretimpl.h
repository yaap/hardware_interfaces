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

#include <aidl/android/hardware/security/sharedsecret/BnSharedSecret.h>
#include <aidl/android/hardware/security/sharedsecret/ISharedSecret.h>
#include <android-base/logging.h>
#include <android-base/result.h>
#include <android/hardware/security/sharedsecret/BnSharedSecret.h>
#include <binder/RpcSession.h>

// We use cpp interfaces to talk to Trusty, and ndk interfaces for the platform
namespace cpp_sharedsecret = android::hardware::security::sharedsecret;
namespace ndk_sharedsecret = aidl::android::hardware::security::sharedsecret;

namespace android {
namespace trusty {
namespace hwcryptohalservice {

class HwCryptoSharedSecret : public ndk_sharedsecret::BnSharedSecret {
  private:
    sp<cpp_sharedsecret::ISharedSecret> mSharedSecretServer;
    sp<IBinder> mRoot;
    sp<RpcSession> mSession;
    android::base::Result<void> connectToTrusty(const char* tipcDev);

  public:
    HwCryptoSharedSecret();

    static std::shared_ptr<HwCryptoSharedSecret> Create(const char* tipcDev);

    ndk::ScopedAStatus getSharedSecretParameters(
            ndk_sharedsecret::SharedSecretParameters* aidl_return) override;

    ndk::ScopedAStatus computeSharedSecret(
            const std::vector<ndk_sharedsecret::SharedSecretParameters>& params,
            std::vector<uint8_t>* aidl_return) override;
};

}  // namespace hwcryptohalservice
}  // namespace trusty
}  // namespace android
