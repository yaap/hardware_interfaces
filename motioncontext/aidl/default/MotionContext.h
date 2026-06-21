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

#include <aidl/android/hardware/motioncontext/BnMotionContext.h>
#include <aidl/android/hardware/motioncontext/BnMotionContextClient.h>
#include <aidl/android/hardware/motioncontext/MotionSubscription.h>

#include <condition_variable>
#include <mutex>
#include <vector>

namespace aidl::android::hardware::motioncontext::impl {

using ::ndk::ScopedAStatus;
using ::std::shared_ptr;

class MotionContextClient;

class MotionContext : public BnMotionContext {
  public:
    MotionContext();
    ~MotionContext();

    ScopedAStatus registerClient(const shared_ptr<IMotionContextCallback>& in_callback,
                                 shared_ptr<IMotionContextClient>* _aidl_return) override;
    void removeClient(const MotionContextClient* client);

    MotionEvent getCurrentMotionEvent();

  private:
    MotionState mCurrentState = MotionState::UNKNOWN;
    MotionState mPreviousState = MotionState::UNKNOWN;
    int64_t mLastStateChangeTimeMs;
    std::mutex mServiceMutex;
    std::vector<std::weak_ptr<MotionContextClient>> mClients;
    AIBinder_DeathRecipient* mDeathRecipient;
};

class MotionContextClient : public BnMotionContextClient,
                            public std::enable_shared_from_this<MotionContextClient> {
  public:
    MotionContextClient(const shared_ptr<IMotionContextCallback>& callback, MotionContext* service);

    ::ndk::ScopedAStatus configureMotionSubscription(
            const std::vector<MotionSubscription>& in_subscriptions, int32_t in_cookie) override;

    ::ndk::ScopedAStatus ackEvent(int sequenceNumber) override;

    void onBinderDeath();
    void onMotionEvent(const MotionEvent& event);
    bool hasSubscriptionForState(MotionState state);

  private:
    shared_ptr<IMotionContextCallback> mCallback;
    MotionContext* mService;
    std::mutex mClientMutex;
    std::vector<MotionSubscription> mSubscriptions;
};

}  // namespace aidl::android::hardware::motioncontext::impl
