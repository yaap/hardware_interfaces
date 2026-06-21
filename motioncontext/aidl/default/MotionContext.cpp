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

#include "MotionContext.h"
#include <android-base/logging.h>
#include <utils/SystemClock.h>
#include <chrono>
#include <ctime>
#include <random>
#include <thread>

using ::aidl::android::hardware::motioncontext::EventDeliveryReason;
using ::ndk::ScopedAStatus;
using namespace std::chrono_literals;

namespace aidl::android::hardware::motioncontext::impl {

namespace {
//! Mutex used to ensure callbacks are called after the initial function returns.
std::mutex gCallbackMutex;

int64_t getTimeInMillis() {
    return ::android::elapsedRealtimeNano() / 1000000;
}
}  // anonymous namespace

void binderDied(void* context) {
    MotionContextClient* client = static_cast<MotionContextClient*>(context);
    client->onBinderDeath();
}

MotionContext::MotionContext() : mLastStateChangeTimeMs(getTimeInMillis()) {
    mDeathRecipient = AIBinder_DeathRecipient_new(binderDied);
}

MotionContext::~MotionContext() {
    AIBinder_DeathRecipient_delete(mDeathRecipient);
}

ScopedAStatus MotionContext::registerClient(const shared_ptr<IMotionContextCallback>& in_callback,
                                            shared_ptr<IMotionContextClient>* _aidl_return) {
    if (in_callback == nullptr) {
        return ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT, "Callback is null");
    }
    shared_ptr<MotionContextClient> client =
            ndk::SharedRefBase::make<MotionContextClient>(in_callback, this);

    std::lock_guard<std::mutex> lock(mServiceMutex);
    mClients.push_back(client);
    AIBinder_linkToDeath(in_callback->asBinder().get(), mDeathRecipient, client.get());
    *_aidl_return = client;
    return ScopedAStatus::ok();
}

void MotionContext::removeClient(const MotionContextClient* client) {
    std::lock_guard<std::mutex> lock(mServiceMutex);
    mClients.erase(std::remove_if(mClients.begin(), mClients.end(),
                                  [client](const std::weak_ptr<MotionContextClient>& weakClient) {
                                      return weakClient.expired() ||
                                             weakClient.lock().get() == client;
                                  }),
                   mClients.end());
}

MotionEvent MotionContext::getCurrentMotionEvent() {
    int64_t now = getTimeInMillis();
    MotionEvent event = {
            .previousState = mPreviousState,
            .currentState = mCurrentState,
            .durationMs = static_cast<int32_t>(now - mLastStateChangeTimeMs),
            .deliveryReason = EventDeliveryReason::MOTION_SUBSCRIPTION_CONFIGURED,
    };
    return event;
}

MotionContextClient::MotionContextClient(const shared_ptr<IMotionContextCallback>& callback,
                                         MotionContext* service)
    : mCallback(callback), mService(service) {}

ScopedAStatus MotionContextClient::configureMotionSubscription(
        const std::vector<MotionSubscription>& in_subscriptions, int32_t /*in_cookie*/) {
    mSubscriptions = in_subscriptions;

    // Per AIDL spec, send current state upon configuration
    MotionEvent event = mService->getCurrentMotionEvent();
    event.deliveryReason = EventDeliveryReason::MOTION_SUBSCRIPTION_CONFIGURED;

    // Avoid immediate callback of client
    std::unique_lock<std::mutex> lock(gCallbackMutex);
    std::thread{[cb = mCallback, event]() {
        std::unique_lock<std::mutex> lock(gCallbackMutex);
        if (cb != nullptr) {
            cb->onMotionEvent(event);
        }
    }}.detach();

    return ScopedAStatus::ok();
}

::ndk::ScopedAStatus MotionContextClient::ackEvent(int /*sequenceNumber*/) {
    return ScopedAStatus::ok();
}

void MotionContextClient::onBinderDeath() {
    mService->removeClient(this);
}

}  // namespace aidl::android::hardware::motioncontext::impl
