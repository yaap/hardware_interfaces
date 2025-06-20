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

#include <android-base/result.h>
#include <android/binder_auto_utils.h>
#include <binder/IBinder.h>
#include <binder/RpcSession.h>
#include <binder/RpcTrusty.h>
#include <binder/Status.h>
#include <trusty/tipc.h>

void parseDeviceName(int argc, char* argv[], char*& device_name, const char* app_name);

namespace android {
namespace trusty {
namespace hwcryptohalservice {

inline ndk::ScopedAStatus convertStatus(android::binder::Status status) {
    if (status.isOk()) {
        return ndk::ScopedAStatus::ok();
    } else {
        auto exCode = status.exceptionCode();
        if (exCode == android::binder::Status::Exception::EX_SERVICE_SPECIFIC) {
            return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                    status.serviceSpecificErrorCode(), status.exceptionMessage());
        } else {
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(exCode,
                                                                    status.exceptionMessage());
        }
    }
}

template <typename T>
android::base::Result<void> connectToTrustyService(const char* tipcDev, const char* portName,
                                                   android::sp<android::RpcSession>& session,
                                                   android::sp<android::IBinder>& root,
                                                   android::sp<T>& serviceServer) {
    assert(!session);
    auto session_initializer = [](android::sp<android::RpcSession>& lSession) {
        lSession->setFileDescriptorTransportMode(
                android::RpcSession::FileDescriptorTransportMode::TRUSTY);
    };
    session = RpcTrustyConnectWithSessionInitializer(tipcDev, portName, session_initializer);
    if (!session) {
        return android::base::ErrnoError() << "failed to connect to trusty service " << portName;
    }
    root = session->getRootObject();
    serviceServer = T::asInterface(root);
    return {};
}

}  // namespace hwcryptohalservice
}  // namespace trusty
}  // namespace android